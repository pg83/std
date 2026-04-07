#include "ares.h"
#include "iface.h"
#include "config.h"
#include "record.h"
#include "result.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/thr/coro.h>
#include <std/sym/i_map.h>
#include <std/thr/event.h>
#include <std/thr/mutex.h>
#include <std/thr/guard.h>
#include <std/alg/defer.h>
#include <std/lib/vector.h>
#include <std/thr/poll_fd.h>
#include <std/thr/parker.h>
#include <std/lib/visitor.h>
#include <std/mem/obj_pool.h>
#include <std/thr/reactor_poll.h>

#if __has_include(<ares.h>)
    #include <ares.h>
#endif

#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

#if __has_include(<ares.h>)
namespace {
    struct DnsResolverImpl;

    struct DnsRecordImpl: public DnsRecord {
        DnsRecordImpl(ObjPool* pool, struct ares_addrinfo_node* node);
    };

    struct DnsResultImpl: public DnsResult {
        DnsResultImpl(ObjPool* pool, int status, struct ares_addrinfo* ai);
        StringView errorDescr() const noexcept override;
    };

    struct DnsRequest: public IntrusiveNode {
        ObjPool* pool;
        Event* event = nullptr;
        DnsResult* result = nullptr;
        bool submitted = false;
        const char* name;

        void complete(int status, struct ares_addrinfo* ai);

        static void callback(void* arg, int status, int, struct ares_addrinfo* ai) {
            ((DnsRequest*)arg)->complete(status, ai);
        }
    };

    struct DnsResolverImpl: public DnsResolver {
        CoroExecutor* exec_;
        ares_channel channel_;
        struct ares_addrinfo_hints hints_;
        Mutex lock_;
        bool driving_ = false;
        IntrusiveList pending_;
        IntrusiveList waiters_;
        IntMap<PollFD> fdMap_;
        Vector<PollFD> fds_;
        ObjPool::Ref pollPool_;
        PollGroup* pollGroup_ = nullptr;
        Parker parker_;

        DnsResolverImpl(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg);
        ~DnsResolverImpl() noexcept;

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;

        void rebuildFds();
        void submitPending();
        void driverLoop(DnsRequest& req);
        void onSockState(ares_socket_t fd, int readable, int writable);

        static void sockStateCb(void* data, ares_socket_t fd, int readable, int writable) {
            ((DnsResolverImpl*)data)->onSockState(fd, readable, writable);
        }
    };
}

DnsRecordImpl::DnsRecordImpl(ObjPool* pool, struct ares_addrinfo_node* node) {
    family = node->ai_family;
    addr = node->ai_addr;

    if (node->ai_next) {
        next = pool->make<DnsRecordImpl>(pool, node->ai_next);
    } else {
        next = nullptr;
    }
}

StringView DnsResultImpl::errorDescr() const noexcept {
    return ares_strerror(error);
}

DnsResultImpl::DnsResultImpl(ObjPool* pool, int status, struct ares_addrinfo* ai) {
    if (ai) {
        atExit(pool, [ai] {
            ares_freeaddrinfo(ai);
        });
    }

    if (status == ARES_ENOTFOUND) {
        error = 0;
        record = nullptr;
    } else if (status == ARES_EBADNAME) {
        error = 0;
        record = nullptr;
    } else if (status == ARES_ENODATA) {
        error = 0;
        record = nullptr;
    } else if (status != ARES_SUCCESS) {
        error = status;
        record = nullptr;
    } else if (!ai) {
        error = 0;
        record = nullptr;
    } else if (!ai->nodes) {
        error = 0;
        record = nullptr;
    } else {
        error = 0;
        record = pool->make<DnsRecordImpl>(pool, ai->nodes);
    }
}

void DnsRequest::complete(int status, struct ares_addrinfo* ai) {
    result = pool->make<DnsResultImpl>(pool, status, ai);

    if (event) {
        remove();
        event->signal();
    }
}

void DnsResolverImpl::onSockState(ares_socket_t fd, int readable, int writable) {
    u32 flags = (readable ? PollFlag::In : 0) | (writable ? PollFlag::Out : 0);

    if (flags) {
        fdMap_.insert(fd, (int)fd, flags);
    } else {
        fdMap_.erase(fd);
    }
}

DnsResolverImpl::DnsResolverImpl(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg)
    : exec_(exec)
    , lock_(Mutex::spinLock(exec))
    , fdMap_(ObjPool::create(pool))
    , pollPool_(ObjPool::fromMemory())
{
    ares_options opts;
    memset(&opts, 0, sizeof(opts));

    opts.timeout = cfg.timeout;
    opts.tries = cfg.tries;
    opts.udp_max_queries = cfg.udpMaxQueries;
    opts.sock_state_cb = sockStateCb;
    opts.sock_state_cb_data = this;

    int optmask = ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_UDP_MAX_QUERIES | ARES_OPT_SOCK_STATE_CB;

    if (cfg.tcp) {
        opts.flags = ARES_FLAG_USEVC | ARES_FLAG_STAYOPEN;
        optmask |= ARES_OPT_FLAGS;
    }

    ares_init_options(&channel_, &opts, optmask);

    if (cfg.server.length()) {
        ares_set_servers_ports_csv(channel_, (const char*)pool->intern(cfg.server).data());
    }

    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = cfg.family;
}

DnsResolverImpl::~DnsResolverImpl() noexcept {
    ares_destroy(channel_);
}

DnsResult* DnsResolverImpl::resolve(ObjPool* pool, const StringView& name) {
    Event ev(exec_);
    DnsRequest req;

    req.pool = pool;
    req.event = &ev;
    req.name = (const char*)pool->intern(name).data();

    lock_.lock();

    if (driving_) {
        pending_.pushBack(&req);

        req.event->wait(makeRunable([this] {
            lock_.unlock();
            parker_.unpark();
        }));

        if (req.result) {
            return req.result;
        }
    } else {
        driving_ = true;
        lock_.unlock();
    }

    req.event = nullptr;
    driverLoop(req);

    return req.result;
}

void DnsResolverImpl::submitPending() {
    IntrusiveList batch;

    LockGuard(lock_).run([&] {
        batch.xchg(pending_);
    });

    while (auto r = (DnsRequest*)batch.popFrontOrNull()) {
        waiters_.pushBack(r);
        ares_getaddrinfo(channel_, r->name, nullptr, &hints_, DnsRequest::callback, r);
        r->submitted = true;
    }
}

void DnsResolverImpl::rebuildFds() {
    fds_.clear();

    fdMap_.visit([this](PollFD pfd) {
        fds_.pushBack(pfd);
    });

    fds_.pushBack({parker_.fd(), PollFlag::In});

    pollPool_ = ObjPool::fromMemory();
    pollGroup_ = PollGroup::create(pollPool_.mutPtr(), fds_.data(), fds_.length());
}

void DnsResolverImpl::driverLoop(DnsRequest& req) {
    if (!req.submitted) {
        ares_getaddrinfo(channel_, req.name, nullptr, &hints_, DnsRequest::callback, &req);
        req.submitted = true;
    }

    while (!req.result) {
        parker_.park([&] {
            submitPending();
            rebuildFds();

            struct timeval tv;

            ares_timeout(channel_, nullptr, &tv);

            // clang-format off
            exec_->poll(pollGroup_, makeVisitor([this](void* ptr) {
                auto ev = (PollFD*)ptr;

                if (ev->fd == parker_.fd()) {
                    parker_.drain();
                } else {
                    ares_socket_t rfd = (ev->flags & PollFlag::In) ? ev->fd : ARES_SOCKET_BAD;
                    ares_socket_t wfd = (ev->flags & PollFlag::Out) ? ev->fd : ARES_SOCKET_BAD;

                    ares_process_fd(channel_, rfd, wfd);
                }
            }), monotonicNowUs() + (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec);
            // clang-format on
        });

        ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    }

    lock_.lock();

    if (auto next = (DnsRequest*)waiters_.popFrontOrNull()) {
        lock_.unlock();
        next->event->signal();
    } else if (auto next = (DnsRequest*)pending_.popFrontOrNull()) {
        lock_.unlock();
        next->event->signal();
    } else {
        driving_ = false;
        lock_.unlock();
    }
}

DnsResolver* stl::createAresResolver(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg) {
    return pool->make<DnsResolverImpl>(pool, exec, cfg);
}
#else
DnsResolver* stl::createAresResolver(ObjPool*, CoroExecutor*, const DnsConfig&) {
    return nullptr;
}
#endif
