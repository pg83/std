#include "dns_ares.h"
#include "dns_iface.h"

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
#include <std/thr/poller.h>
#include <std/thr/poll_fd.h>
#include <std/sys/event_fd.h>
#include <std/mem/obj_pool.h>

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
        Vector<PollFD> outFds_;
        EventFD wakeup_;

        DnsResolverImpl(ObjPool* pool, CoroExecutor* exec);
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

DnsResolverImpl::DnsResolverImpl(ObjPool* pool, CoroExecutor* exec)
    : exec_(exec)
    , lock_(Mutex::spinLock(exec))
    , fdMap_(ObjPool::create(pool))
{
    ares_options opts;
    memset(&opts, 0, sizeof(opts));

    opts.timeout = 500;
    opts.tries = 3;
    opts.sock_state_cb = sockStateCb;
    opts.sock_state_cb_data = this;

    ares_init_options(&channel_, &opts, ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_SOCK_STATE_CB);

    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = AF_UNSPEC;
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
            wakeup_.signal();
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

    fds_.pushBack({wakeup_.fd(), PollFlag::In});
}

void DnsResolverImpl::driverLoop(DnsRequest& req) {
    if (!req.submitted) {
        ares_getaddrinfo(channel_, req.name, nullptr, &hints_, DnsRequest::callback, &req);
        req.submitted = true;
    }

    while (!req.result) {
        submitPending();
        rebuildFds();

        struct timeval tv;

        ares_timeout(channel_, nullptr, &tv);

        u64 deadlineUs = monotonicNowUs() + (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;
        outFds_.grow(fds_.length());
        size_t nout = exec_->pollMulti(fds_.data(), outFds_.mutData(), fds_.length(), deadlineUs);

        for (size_t i = 0; i < nout; ++i) {
            if (outFds_[i].fd == wakeup_.fd()) {
                wakeup_.drain();
            } else {
                ares_socket_t rfd = (outFds_[i].flags & PollFlag::In) ? outFds_[i].fd : ARES_SOCKET_BAD;
                ares_socket_t wfd = (outFds_[i].flags & PollFlag::Out) ? outFds_[i].fd : ARES_SOCKET_BAD;

                ares_process_fd(channel_, rfd, wfd);
            }
        }

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

DnsResolver* stl::createAresResolver(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<DnsResolverImpl>(pool, exec);
}
#else
DnsResolver* stl::createAresResolver(ObjPool*, CoroExecutor*) {
    return nullptr;
}
#endif
