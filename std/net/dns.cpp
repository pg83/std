#include "dns.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/lib/vector.h>
#include <std/sym/i_map.h>
#include <std/thr/coro.h>
#include <std/thr/event.h>
#include <std/thr/mutex.h>
#include <std/alg/defer.h>
#include <std/thr/poller.h>
#include <std/mem/obj_pool.h>

#include <ares.h>

#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    template <typename F>
    static void reg(ObjPool* pool, F f) {
        pool->make<ScopedGuard<F>>(f);
    }

    struct DnsResolverImpl;

    struct DnsRecordImpl: public DnsRecord {
        DnsRecordImpl(ObjPool* pool, struct ares_addrinfo_node* node);
    };

    struct DnsResultImpl: public DnsResult {
        DnsResultImpl(ObjPool* pool, int status, struct ares_addrinfo* ai);
    };

    struct DnsRequest: public IntrusiveNode {
        ObjPool* pool;
        Event* event = nullptr;
        DnsResult* result = nullptr;

        void complete(int status, struct ares_addrinfo* ai);

        static void callback(void* arg, int status, int, struct ares_addrinfo* ai) {
            ((DnsRequest*)arg)->complete(status, ai);
        }
    };

    struct DnsResolverImpl: public DnsResolver {
        CoroExecutor* exec_;
        ares_channel channel_;
        Mutex lock_;
        bool driving_ = false;
        IntrusiveList waiters_;
        IntMap<int> fdMap_;
        Vector<int> fds_;

        DnsResolverImpl(ObjPool* pool, CoroExecutor* exec);
        ~DnsResolverImpl() noexcept;

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;

        void driverLoop(DnsRequest& req);
        void rebuildFds();

        static void sockStateCb(void* data, ares_socket_t fd, int readable, int writable) {
            auto self = (DnsResolverImpl*)data;

            if (readable || writable) {
                self->fdMap_.insert((u64)fd, (int)fd);
            } else {
                self->fdMap_.erase((u64)fd);
            }
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

DnsResultImpl::DnsResultImpl(ObjPool* pool, int status, struct ares_addrinfo* ai) {
    if (ai) {
        reg(pool, [ai] {
            ares_freeaddrinfo(ai);
        });
    }

    if (status == ARES_ENOTFOUND) {
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
        event->signal();
    }
}

DnsResolverImpl::DnsResolverImpl(ObjPool* pool, CoroExecutor* exec)
    : exec_(exec)
    , lock_(Mutex::spinLock(exec))
    , fdMap_(pool)
{
    ares_options opts;

    memset(&opts, 0, sizeof(opts));
    opts.timeout = 5000;
    opts.tries = 3;
    opts.sock_state_cb = sockStateCb;
    opts.sock_state_cb_data = this;

    ares_init_options(&channel_, &opts, ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_SOCK_STATE_CB);
}

DnsResolverImpl::~DnsResolverImpl() noexcept {
    ares_destroy(channel_);
}

DnsResult* DnsResolverImpl::resolve(ObjPool* pool, const StringView& name) {
    DnsRequest req;

    req.pool = pool;

    struct ares_addrinfo_hints hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    auto zname = pool->intern(name);

    lock_.lock();
    ares_getaddrinfo(channel_, (const char*)zname.data(), nullptr, &hints, DnsRequest::callback, &req);

    if (req.result) {
        lock_.unlock();

        return req.result;
    }

    if (driving_) {
        Event ev(exec_);

        req.event = &ev;
        waiters_.pushBack(&req);
        ev.wait(makeRunable([this] {
            lock_.unlock();
        }));

        if (req.result) {
            return req.result;
        }

        // woken as new driver, driving_ already true
    } else {
        driving_ = true;
        lock_.unlock();
    }

    driverLoop(req);

    return req.result;
}

void DnsResolverImpl::rebuildFds() {
    fds_.clear();
    fdMap_.visit([this](int fd) {
        fds_.pushBack(fd);
    });
}

void DnsResolverImpl::driverLoop(DnsRequest& req) {
    while (!req.result) {
        rebuildFds();

        struct timeval tv;

        ares_timeout(channel_, nullptr, &tv);

        u64 deadlineUs = monotonicNowUs() + (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;

        exec_->pollMulti(fds_.mutData(), fds_.length(), PollFlag::In | PollFlag::Out, deadlineUs);

        for (size_t i = 0; i < fds_.length(); ++i) {
            ares_process_fd(channel_, fds_[i], fds_[i]);
        }

        ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    }

    lock_.lock();

    if (auto next = (DnsRequest*)waiters_.popFrontOrNull(); next) {
        lock_.unlock();
        next->event->signal();
    } else {
        driving_ = false;
        lock_.unlock();
    }
}

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<DnsResolverImpl>(pool, exec);
}
