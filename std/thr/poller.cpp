#include "poller.h"
#include "poll_fd.h"
#include "pollable.h"

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

    struct EpollPoller: public WaitablePoller {
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

        int fd() override {
            return epfd_;
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
    struct KqueuePoller: public WaitablePoller {
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

        int fd() override {
            return kqfd_;
        }
    };
}
#endif

namespace {
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
                    .events = e.toPollEvents(),
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

                PollFD ev{pfd.fd, PollFD::fromPollEvents(pfd.revents)};

                v.visit(&ev);
            }
        }

    };

    struct HybridPoller: public PollerIface {
        static constexpr size_t threshold = 100;

        PollPoller* poll_;
        PollerIface* current_;
        PollerIface* native_;

        HybridPoller(ObjPool* pool, PollerIface* native)
            : poll_(pool->make<PollPoller>(pool))
            , current_(poll_)
            , native_(native)
        {
        }

        void migrate() {
            poll_->armed_.visit([&](const PollFD& pfd) {
                native_->arm(pfd);
            });

            current_ = native_;
            poll_ = nullptr;
        }

        void arm(PollFD pfd) override {
            if (poll_) {
                poll_->arm(pfd);

                if (poll_->armed_.size() > threshold) {
                    migrate();
                }
            } else {
                current_->arm(pfd);
            }
        }

        void disarm(int fd) override {
            current_->disarm(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            current_->waitImpl(v, timeoutUs);
        }
    };
}

PollerIface* PollerIface::create(ObjPool* pool) {
    if (getenv("USE_POLL_POLLER")) {
        return pool->make<PollPoller>(pool);
    }

    if (auto native = WaitablePoller::create(pool); native) {
        return pool->make<HybridPoller>(pool, native);
    }

    return pool->make<PollPoller>(pool);
}

WaitablePoller* WaitablePoller::create(ObjPool* pool) {
#if defined(__linux__)
    return pool->make<EpollPoller>();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return pool->make<KqueuePoller>();
#else
    return nullptr;
#endif
}

namespace {
    struct SlavePoller: public WaitablePoller {
        WaitablePoller* slave_;
        Pollable* reactor_;

        SlavePoller(WaitablePoller* slave, Pollable* reactor)
            : slave_(slave)
            , reactor_(reactor)
        {
        }

        void arm(PollFD pfd) override {
            slave_->arm(pfd);
        }

        void disarm(int fd) override {
            slave_->disarm(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            auto deadlineUs = monotonicNowUs() + (u64)timeoutUs;

            if (!reactor_->poll({slave_->fd(), PollFlag::In}, deadlineUs)) {
                return;
            }

            slave_->waitImpl(v, 0);
        }

        int fd() override {
            return slave_->fd();
        }
    };
}

WaitablePoller* WaitablePoller::create(ObjPool* pool, Pollable* reactor) {
    if (auto native = WaitablePoller::create(pool); native) {
        return pool->make<SlavePoller>(native, reactor);
    }

    return nullptr;
}

void PollerIface::waitBase(VisitorFace&& v, u64 deadlineUs) {
    if (auto now = monotonicNowUs(); now >= deadlineUs) {
        waitImpl(v, 0);
    } else {
        waitImpl(v, min(deadlineUs - now, (u64)30000000));
    }
}
