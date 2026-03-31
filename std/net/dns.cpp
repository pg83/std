#include "dns.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
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
    struct DnsResolverImpl;

    struct DnsRequest: public IntrusiveNode {
        DnsResolverImpl* resolver;
        ObjPool* pool;
        Event* event;
        DnsResult result;
        bool ready = false;

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
        DnsRequest* driverReq_ = nullptr;
        IntrusiveList waiters_;

        DnsResolverImpl(CoroExecutor* exec);
        ~DnsResolverImpl() noexcept;

        DnsResult resolve(ObjPool* pool, const StringView& name) override;

        void driverLoop(DnsRequest& req);
    };
}

void DnsRequest::complete(int status, struct ares_addrinfo* ai) {
    STD_DEFER {
        if (ai) {
            ares_freeaddrinfo(ai);
        }
    };

    if (status != ARES_SUCCESS) {
        result.error = status;
        result.addr = nullptr;
        result.addrLen = 0;
    } else {
        auto node = ai->nodes;

        if (node) {
            result.error = 0;

            if (node->ai_family == AF_INET) {
                auto dst = (struct sockaddr_in*)pool->allocate(sizeof(struct sockaddr_in));

                memcpy(dst, node->ai_addr, sizeof(struct sockaddr_in));
                result.addr = (sockaddr*)dst;
                result.addrLen = sizeof(struct sockaddr_in);
            } else {
                auto dst = (struct sockaddr_in6*)pool->allocate(sizeof(struct sockaddr_in6));

                memcpy(dst, node->ai_addr, sizeof(struct sockaddr_in6));
                result.addr = (sockaddr*)dst;
                result.addrLen = sizeof(struct sockaddr_in6);
            }
        } else {
            result.error = ARES_ENODATA;
            result.addr = nullptr;
            result.addrLen = 0;
        }
    }

    ready = true;

    if (this != resolver->driverReq_) {
        event->signal();
    }
}

DnsResolverImpl::DnsResolverImpl(CoroExecutor* exec)
    : exec_(exec)
    , lock_(Mutex::spinLock(exec))
{
    ares_options opts;

    memset(&opts, 0, sizeof(opts));
    opts.timeout = 5000;
    opts.tries = 3;

    ares_init_options(&channel_, &opts, ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES);
}

DnsResolverImpl::~DnsResolverImpl() noexcept {
    ares_destroy(channel_);
}

DnsResult DnsResolverImpl::resolve(ObjPool* pool, const StringView& name) {
    DnsRequest req;
    Event ev(exec_);

    req.resolver = this;
    req.pool = pool;
    req.event = &ev;

    struct ares_addrinfo_hints hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    auto zname = pool->intern(name);

    lock_.lock();

    auto prevDriver = driverReq_;

    driverReq_ = &req;
    ares_getaddrinfo(channel_, (const char*)zname.data(), nullptr, &hints, DnsRequest::callback, &req);
    driverReq_ = prevDriver;

    if (req.ready) {
        lock_.unlock();

        return req.result;
    }

    if (driving_) {
        waiters_.pushBack(&req);
        ev.wait(makeRunable([this] {
            lock_.unlock();
        }));

        if (req.ready) {
            return req.result;
        }

        // woken as new driver, driving_ already true
    } else {
        driving_ = true;
        driverReq_ = &req;
        lock_.unlock();
    }

    driverLoop(req);

    return req.result;
}

void DnsResolverImpl::driverLoop(DnsRequest& req) {
    driverReq_ = &req;

    while (!req.ready) {
        ares_socket_t fds[ARES_GETSOCK_MAXNUM];
        int bitmask = ares_getsock(channel_, fds, ARES_GETSOCK_MAXNUM);

        int nfds = 0;

        for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i) {
            if (ARES_GETSOCK_READABLE(bitmask, i) || ARES_GETSOCK_WRITABLE(bitmask, i)) {
                ++nfds;
            } else {
                break;
            }
        }

        if (nfds > 0) {
            int intFds[ARES_GETSOCK_MAXNUM];

            for (int i = 0; i < nfds; ++i) {
                intFds[i] = (int)fds[i];
            }

            struct timeval tv;

            ares_timeout(channel_, nullptr, &tv);

            u64 deadlineUs = monotonicNowUs() + (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;

            exec_->pollMulti(intFds, (size_t)nfds, PollFlag::In | PollFlag::Out, deadlineUs);
        } else {
            struct timeval tv;

            ares_timeout(channel_, nullptr, &tv);

            u64 timeoutUs = (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;

            exec_->sleepTout(timeoutUs);
        }

        ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

        // also process each fd
        for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i) {
            if (ARES_GETSOCK_READABLE(bitmask, i) || ARES_GETSOCK_WRITABLE(bitmask, i)) {
                ares_socket_t rfd = ARES_GETSOCK_READABLE(bitmask, i) ? fds[i] : ARES_SOCKET_BAD;
                ares_socket_t wfd = ARES_GETSOCK_WRITABLE(bitmask, i) ? fds[i] : ARES_SOCKET_BAD;

                ares_process_fd(channel_, rfd, wfd);
            } else {
                break;
            }
        }
    }

    lock_.lock();

    if (auto next = (DnsRequest*)waiters_.popFrontOrNull(); next) {
        driverReq_ = next;
        lock_.unlock();
        next->event->signal();
    } else {
        driving_ = false;
        driverReq_ = nullptr;
        lock_.unlock();
    }
}

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<DnsResolverImpl>(exec);
}
