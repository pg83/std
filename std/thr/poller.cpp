#include "poller.h"

#include <std/sys/fd.h>
#include <std/sys/types.h>
#include <std/alg/range.h>
#include <std/sym/i_map.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>

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

        u32 wait(PollEvent* out, u32 maxEvents, u32 timeoutUs) override {
            epoll_event raw[64];
            u32 cap = maxEvents < 64 ? maxEvents : 64;

            int n = epoll_wait(epfd_, raw, (int)cap, (int)((timeoutUs + 999) / 1000));

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
                EV_SET(&changes[n++], fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, data);
            }

            if (flags & PollFlag::Out) {
                EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, data);
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

        u32 wait(PollEvent* out, u32 maxEvents, u32 timeoutUs) override {
            struct kevent raw[64];

            u32 cap = maxEvents < 64 ? maxEvents : 64;

            struct timespec ts;

            ts.tv_sec = timeoutUs / 1000000;
            ts.tv_nsec = (timeoutUs % 1000000) * 1000L;

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
        ScopedFD wakeReadFd_;
        ScopedFD wakeWriteFd_;

        struct Cmd {
            int fd;
            u32 flags; // 0 = disarm
            void* data;
        };

        // reactor-thread-only
        IntMap<Cmd> armed_;
        Vector<struct pollfd> fds_; // rebuilt each wait()

        PollPoller() {
            createPipeFD(wakeReadFd_, wakeWriteFd_);
            fcntl(wakeReadFd_.get(), F_SETFL, O_NONBLOCK);
        }

        ~PollPoller() noexcept {
        }

        void arm(int fd, u32 flags, void* data) override {
            Cmd cmd{
                .fd = fd,
                .flags = flags,
                .data = data,
            };

            wakeWriteFd_.write(&cmd, sizeof(cmd));
        }

        void disarm(int fd) override {
            Cmd cmd{
                .fd = fd,
                .flags = 0,
                .data = nullptr,
            };

            wakeWriteFd_.write(&cmd, sizeof(cmd));
        }

        // Read all pending Cmds from pipe, apply to armed_. Returns true if any read.
        void drainCmds() {
            Cmd batch[16];
            ssize_t n;

            while ((n = ::read(wakeReadFd_.get(), batch, sizeof(batch))) > 0) {
                STD_INSIST((size_t)n % sizeof(Cmd) == 0);

                for (const Cmd& cmd : range(batch, batch + (size_t)n / sizeof(Cmd))) {
                    if (cmd.flags != 0) {
                        armed_[cmd.fd] = cmd;
                    } else {
                        armed_.erase(cmd.fd);
                    }
                }
            }
        }

        void buildFds() {
            fds_.clear();

            fds_.pushBack({
                .fd = wakeReadFd_.get(),
                .events = POLLIN,
            });

            armed_.visit([&](const Cmd& cmd) {
                fds_.pushBack({
                    .fd = cmd.fd,
                    .events = toPollEvents(cmd.flags),
                });
            });
        }

        u32 wait(PollEvent* out, u32 maxEvents, u32 timeoutUs) override {
            drainCmds();
            buildFds();

            int n = STD_POLL(fds_.mutData(), (int)fds_.length(), (int)((timeoutUs + 999) / 1000));

            if (n < 0) {
                return 0;
            }

            u32 count = 0;

            for (size_t i = 1; i < fds_.length() && count < maxEvents; ++i) {
                if (fds_[i].revents == 0) {
                    continue;
                }

                int efd = fds_[i].fd;

                if (Cmd* cmd = armed_.find(efd); cmd) {
                    out[count].data = cmd->data;
                    out[count].flags = fromPollEvents(fds_[i].revents);
                    ++count;
                    armed_.erase((u64)efd); // ONESHOT
                }
            }

            return count;
        }
    };
}

PollerIface* PollerIface::create() {
    if (getenv("USE_POLL_POLLER")) {
        return new PollPoller();
    }

#if defined(__linux__)
    return new EpollPoller();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return new KqueuePoller();
#else
    return new PollPoller();
#endif
}
