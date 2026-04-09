#include "io_reactor.h"

#include "coro.h"
#include "pool.h"
#include "poll_fd.h"
#include "reactor_poll.h"

#include <std/mem/obj_pool.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
    #define MSG_NOSIGNAL 0
#endif

#ifndef SOCK_NONBLOCK
    #define SOCK_NONBLOCK 0
#endif

#ifndef SOCK_CLOEXEC
    #define SOCK_CLOEXEC 0
#endif

using namespace stl;

namespace {
    struct PollIoReactor: public IoReactor {
        ReactorIface* reactor_;
        CoroExecutor* exec_;
        ThreadPool* offload_;

        PollIoReactor(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, ThreadPool* offload);

        ssize_t recv(int fd, void* buf, size_t len, u64 deadlineUs) override;
        ssize_t send(int fd, const void* buf, size_t len, u64 deadlineUs) override;
        int accept(int fd, sockaddr* addr, u32* addrLen, u64 deadlineUs) override;
        int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) override;

        ssize_t pread(int fd, void* buf, size_t len, off_t offset) override;
        ssize_t pwrite(int fd, const void* buf, size_t len, off_t offset) override;
        int fsync(int fd) override;
        int fdatasync(int fd) override;

        u32 poll(PollFD pfd, u64 deadlineUs) override;
        void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) override;
    };
}

PollIoReactor::PollIoReactor(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, ThreadPool* offload)
    : reactor_(ReactorIface::create(exec, mainPool, pool))
    , exec_(exec)
    , offload_(offload)
{
}

ssize_t PollIoReactor::recv(int fd, void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::recv(fd, buf, len, 0);

        if (n >= 0) {
            return n;
        }

        if (errno != EAGAIN) {
            return -errno;
        }

        if (!reactor_->poll({fd, PollFlag::In}, deadlineUs)) {
            return -EAGAIN;
        }
    }
}

ssize_t PollIoReactor::send(int fd, const void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::send(fd, buf, len, MSG_NOSIGNAL);

        if (n >= 0) {
            return n;
        }

        if (errno != EAGAIN) {
            return -errno;
        }

        if (!reactor_->poll({fd, PollFlag::Out}, deadlineUs)) {
            return -EAGAIN;
        }
    }
}

int PollIoReactor::accept(int fd, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    for (;;) {
        socklen_t slen = addrLen ? (socklen_t)*addrLen : 0;

#ifdef __linux__
        int newFd = ::accept4(fd, addr, addrLen ? &slen : nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
        int newFd = ::accept(fd, addr, addrLen ? &slen : nullptr);
#endif

        if (newFd >= 0) {
            if (addrLen) {
                *addrLen = (u32)slen;
            }

            return newFd;
        }

        if (errno != EAGAIN) {
            return -errno;
        }

        if (!reactor_->poll({fd, PollFlag::In}, deadlineUs)) {
            return -EAGAIN;
        }
    }
}

int PollIoReactor::connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    if (int r = ::connect(fd, addr, addrLen); r == 0) {
        return 0;
    } else if (errno != EINPROGRESS) {
        return -errno;
    }

    reactor_->poll({fd, PollFlag::Out}, deadlineUs);

    int err = 0;
    socklen_t len = sizeof(err);

    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        return -errno;
    }

    if (err) {
        return -err;
    }

    return 0;
}

ssize_t PollIoReactor::pread(int fd, void* buf, size_t len, off_t offset) {
    ssize_t result;

    exec_->offload(offload_, [&] {
        ssize_t n = ::pread(fd, buf, len, offset);
        result = n < 0 ? -errno : n;
    });

    return result;
}

ssize_t PollIoReactor::pwrite(int fd, const void* buf, size_t len, off_t offset) {
    ssize_t result;

    exec_->offload(offload_, [&] {
        ssize_t n = ::pwrite(fd, buf, len, offset);
        result = n < 0 ? -errno : n;
    });

    return result;
}

int PollIoReactor::fsync(int fd) {
    int result;

    exec_->offload(offload_, [&] {
        result = ::fsync(fd) < 0 ? -errno : 0;
    });

    return result;
}

int PollIoReactor::fdatasync(int fd) {
    int result;

    // clang-format off
    exec_->offload(offload_, [&] {
#if defined(__APPLE__)
        result = ::fcntl(fd, F_FULLFSYNC) < 0 ? -errno : 0;
#else
        result = ::fdatasync(fd) < 0 ? -errno : 0;
#endif
    });
    // clang-format on

    return result;
}

u32 PollIoReactor::poll(PollFD pfd, u64 deadlineUs) {
    return reactor_->poll(pfd, deadlineUs);
}

void PollIoReactor::poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) {
    reactor_->poll(g, visitor, deadlineUs);
}

IoReactor* IoReactor::createPoll(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, ThreadPool* offload) {
    return pool->make<PollIoReactor>(pool, exec, mainPool, offload);
}
