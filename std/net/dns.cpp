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
        DnsResolverImpl* resolver;
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
        Mutex lock_;
        bool driving_ = false;
        IntrusiveList pending_;
        IntrusiveList waiters_;
        IntrusiveList ready_;
        IntMap<int> fdMap_;
        Vector<int> fds_;

        DnsResolverImpl(ObjPool* pool, CoroExecutor* exec);
        ~DnsResolverImpl() noexcept;

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;

        void driverLoop(DnsRequest& req);
        void submitPending();
        void rebuildFds();
        void wakeReady();

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
        resolver->ready_.pushBack(this);
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

    req.resolver = this;
    req.pool = pool;
    req.event = nullptr;
    req.name = (const char*)pool->intern(name).data();

    lock_.lock();

    if (driving_) {
        req.event = pool->make<Event>(exec_);
        pending_.pushBack(&req);
        req.event->wait(makeRunable([this] {
            lock_.unlock();
        }));

        if (req.result) {
            return req.result;
        }

        // woken as new driver
        req.event = nullptr;
    } else {
        driving_ = true;
        lock_.unlock();
    }

    driverLoop(req);

    return req.result;
}

void DnsResolverImpl::submitPending() {
    lock_.lock();
    IntrusiveList batch;
    batch.xchg(pending_);
    lock_.unlock();

    struct ares_addrinfo_hints hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    while (auto r = (DnsRequest*)batch.popFrontOrNull()) {
        waiters_.pushBack(r);
        ares_getaddrinfo(channel_, r->name, nullptr, &hints, DnsRequest::callback, r);
        r->submitted = true;
    }
}

void DnsResolverImpl::rebuildFds() {
    fds_.clear();
    fdMap_.visit([this](int fd) {
        fds_.pushBack(fd);
    });
}

void DnsResolverImpl::wakeReady() {
    while (auto ready = (DnsRequest*)ready_.popFrontOrNull()) {
        ready->event->signal();
    }
}

void DnsResolverImpl::driverLoop(DnsRequest& req) {
    if (!req.submitted) {
        struct ares_addrinfo_hints hints;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;

        ares_getaddrinfo(channel_, req.name, nullptr, &hints, DnsRequest::callback, &req);
        req.submitted = true;
        wakeReady();
    }

    while (!req.result) {
        submitPending();
        rebuildFds();

        struct timeval tv;

        ares_timeout(channel_, nullptr, &tv);

        u64 deadlineUs = monotonicNowUs() + (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;

        (void)deadlineUs;
        exec_->pollMulti(fds_.mutData(), fds_.length(), PollFlag::In | PollFlag::Out, deadlineUs);

        for (size_t i = 0; i < fds_.length(); ++i) {
            ares_process_fd(channel_, fds_[i], fds_[i]);
            wakeReady();
        }

        ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
        wakeReady();
    }

    wakeReady();

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

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<DnsResolverImpl>(pool, exec);
}
