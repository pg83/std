#include "poller.h"

#include <std/sys/types.h>
#include <std/dbg/insist.h>

#include <errno.h>
#include <unistd.h>

#if defined(__linux__)
    #include <sys/epoll.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/event.h>
    #include <sys/time.h>
#else
    #error unsupported platform
#endif

using namespace stl;

PollerIface::~PollerIface() noexcept {
}

#if defined(__linux__)
namespace {
    static u32 toEpollFlags(u32 flags) noexcept {
        u32 r = 0;
        if (flags & PollFlag::In) {
            r |= EPOLLIN;
        }
        if (flags & PollFlag::Out) {
            r |= EPOLLOUT;
        }
        if (flags & PollFlag::Err) {
            r |= EPOLLERR;
        }
        if (flags & PollFlag::Hup) {
            r |= EPOLLHUP;
        }
        return r;
    }

    static u32 fromEpollFlags(u32 events) noexcept {
        u32 r = 0;
        if (events & EPOLLIN) {
            r |= PollFlag::In;
        }
        if (events & EPOLLOUT) {
            r |= PollFlag::Out;
        }
        if (events & EPOLLERR) {
            r |= PollFlag::Err;
        }
        if (events & EPOLLHUP) {
            r |= PollFlag::Hup;
        }
        return r;
    }

    struct EpollPoller: public PollerIface {
        int epfd_;

        EpollPoller() {
            epfd_ = epoll_create1(0);
            STD_INSIST(epfd_ >= 0);
        }

        ~EpollPoller() noexcept override {
            ::close(epfd_);
        }

        void arm(int fd, u32 flags, void* data) override {
            epoll_event ev{};
            ev.data.ptr = data;
            ev.events = toEpollFlags(flags) | EPOLLONESHOT;
            if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
                STD_INSIST(errno == ENOENT);
                STD_INSIST(epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == 0);
            }
        }

        void disarm(int fd) override {
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
        }

        u32 wait(PollEvent* out, u32 maxEvents, u32 timeoutMs) override {
            epoll_event raw[64];
            u32 cap = maxEvents < 64 ? maxEvents : 64;
            int n = epoll_wait(epfd_, raw, (int)cap, (int)timeoutMs);
            if (n < 0) {
                return 0;
            }
            for (int i = 0; i < n; i++) {
                out[i].data = raw[i].data.ptr;
                out[i].flags = fromEpollFlags(raw[i].events);
            }
            return (u32)n;
        }
    };
}

PollerIface* PollerIface::create() {
    return new EpollPoller();
}
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
namespace {
    struct KqueuePoller: public PollerIface {
        int kqfd_;

        KqueuePoller() {
            kqfd_ = kqueue();
            STD_INSIST(kqfd_ >= 0);
        }

        ~KqueuePoller() noexcept override {
            ::close(kqfd_);
        }

        void arm(int fd, u32 flags, void* data) override {
            struct kevent changes[2];
            int n = 0;
            if (flags & PollFlag::In) {
                EV_SET(&changes[n++], fd, EVFILT_READ,
                       EV_ADD | EV_ONESHOT, 0, 0, data);
            }
            if (flags & PollFlag::Out) {
                EV_SET(&changes[n++], fd, EVFILT_WRITE,
                       EV_ADD | EV_ONESHOT, 0, 0, data);
            }
            if (n > 0) {
                STD_INSIST(kevent(kqfd_, changes, n, nullptr, 0, nullptr) == 0);
            }
        }

        void disarm(int fd) override {
            struct kevent changes[2];
            EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
            EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
            kevent(kqfd_, changes, 2, nullptr, 0, nullptr);
        }

        u32 wait(PollEvent* out, u32 maxEvents, u32 timeoutMs) override {
            struct kevent raw[64];
            u32 cap = maxEvents < 64 ? maxEvents : 64;
            struct timespec ts;
            ts.tv_sec = timeoutMs / 1000;
            ts.tv_nsec = (timeoutMs % 1000) * 1000000L;
            int n = kevent(kqfd_, nullptr, 0, raw, (int)cap, &ts);
            if (n < 0) {
                return 0;
            }
            for (int i = 0; i < n; i++) {
                out[i].data = raw[i].udata;
                u32 fl = 0;
                if (raw[i].filter == EVFILT_READ) {
                    fl |= PollFlag::In;
                }
                if (raw[i].filter == EVFILT_WRITE) {
                    fl |= PollFlag::Out;
                }
                if (raw[i].flags & EV_EOF) {
                    fl |= PollFlag::Hup;
                }
                if (raw[i].flags & EV_ERROR) {
                    fl |= PollFlag::Err;
                }
                out[i].flags = fl;
            }
            return (u32)n;
        }
    };
}

PollerIface* PollerIface::create() {
    return new KqueuePoller();
}
#endif
