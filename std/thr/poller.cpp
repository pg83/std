#include "poller.h"
#include "poll_fd.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/sys/types.h>
#include <std/alg/range.h>
#include <std/sym/i_map.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>
#include <std/alg/minmax.h>
#include <std/mem/obj_pool.h>

#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#if defined(__linux__)
    #include <sys/epoll.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/event.h>
    #include <sys/time.h>
#endif

#if defined(_WIN32)
    #include <winsock2.h>
    #define STD_POLL WSAPoll
#else
    #define STD_POLL poll
#endif

using namespace stl;

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

        ~EpollPoller() noexcept {
            ::close(epfd_);
        }

        void arm(PollFD pfd) override {
            epoll_event ev{};

            ev.data.fd = pfd.fd;
            ev.events = toEpollFlags(pfd.flags) | EPOLLONESHOT;

            if (epoll_ctl(epfd_, EPOLL_CTL_MOD, pfd.fd, &ev) < 0) {
                STD_INSIST(errno == ENOENT);
                STD_INSIST(epoll_ctl(epfd_, EPOLL_CTL_ADD, pfd.fd, &ev) == 0);
            }
        }

        void disarm(int fd) override {
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            epoll_event raw[1024];

            int n = epoll_wait(epfd_, raw, sizeof(raw) / sizeof(raw[0]), (int)((timeoutUs + 999) / 1000));

            if (n <= 0) {
                return;
            }

            for (auto& e : range(raw, raw + n)) {
                PollFD ev{e.data.fd, fromEpollFlags(e.events)};

                v.visit(&ev);
            }
        }
    };
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

        ~KqueuePoller() noexcept {
            ::close(kqfd_);
        }

        void arm(PollFD pfd) override {
            struct kevent changes[2];

            int n = 0;

            if (pfd.flags & PollFlag::In) {
                EV_SET(&changes[n++], pfd.fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, 0);
            }

            if (pfd.flags & PollFlag::Out) {
                EV_SET(&changes[n++], pfd.fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, 0);
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

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            struct kevent raw[1024];

            struct timespec ts;

            ts.tv_sec = timeoutUs / 1000000;
            ts.tv_nsec = (timeoutUs % 1000000) * 1000L;

            int n = kevent(kqfd_, nullptr, 0, raw, sizeof(raw) / sizeof(raw[0]), &ts);

            if (n <= 0) {
                return;
            }

            for (auto& e : range(raw, raw + n)) {
                u32 fl = 0;

                if (e.filter == EVFILT_READ) {
                    fl |= PollFlag::In;
                }

                if (e.filter == EVFILT_WRITE) {
                    fl |= PollFlag::Out;
                }

                if (e.flags & EV_EOF) {
                    fl |= PollFlag::Hup;
                }

                if (e.flags & EV_ERROR) {
                    fl |= PollFlag::Err;
                }

                PollFD ev{(int)e.ident, fl};

                v.visit(&ev);
            }
        }
    };
}
#endif

namespace {
    static short toPollEvents(u32 flags) noexcept {
        short r = 0;

        if (flags & PollFlag::In) {
            r |= POLLIN;
        }

        if (flags & PollFlag::Out) {
            r |= POLLOUT;
        }

        return r;
    }

    static u32 fromPollEvents(short events) noexcept {
        u32 r = 0;

        if (events & POLLIN) {
            r |= PollFlag::In;
        }

        if (events & POLLOUT) {
            r |= PollFlag::Out;
        }

        if (events & POLLERR) {
            r |= PollFlag::Err;
        }

        if (events & POLLHUP) {
            r |= PollFlag::Hup;
        }

        return r;
    }

    struct PollPoller: public PollerIface {
        IntMap<PollFD> armed_;
        Vector<struct pollfd> fds_; // rebuilt each wait()

        PollPoller(ObjPool* pool)
            : armed_(ObjPool::create(pool))
        {
        }

        void arm(PollFD pfd) override {
            armed_[pfd.fd] = pfd;
        }

        void disarm(int fd) override {
            armed_.erase(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            fds_.clear();

            armed_.visit([&](const PollFD& e) {
                fds_.pushBack({
                    .fd = e.fd,
                    .events = toPollEvents(e.flags),
                    .revents = 0,
                });
            });

            int n = STD_POLL(fds_.mutData(), (int)fds_.length(), (int)((timeoutUs + 999) / 1000));

            if (n < 0) {
                return;
            }

            for (const auto& pfd : range(fds_)) {
                if (pfd.revents == 0) {
                    continue;
                }

                if (!armed_.erase(pfd.fd)) {
                    continue;
                }

                PollFD ev{pfd.fd, fromPollEvents(pfd.revents)};

                v.visit(&ev);
            }
        }
    };
}

PollerIface* PollerIface::create(ObjPool* pool) {
    if (getenv("USE_POLL_POLLER")) {
        return pool->make<PollPoller>(pool);
    }

#if defined(__linux__)
    return pool->make<EpollPoller>();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return pool->make<KqueuePoller>();
#endif

    return pool->make<PollPoller>(pool);
}

void PollerIface::waitBase(VisitorFace&& v, u64 deadlineUs) {
    if (auto now = monotonicNowUs(); now >= deadlineUs) {
        waitImpl(v, 0);
    } else {
        waitImpl(v, min(deadlineUs - now, (u64)30000000));
    }
}
