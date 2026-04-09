#include "io_reactor.h"

#include "coro.h"
#include "pool.h"
#include "poll_fd.h"
#include "reactor_poll.h"

#include <std/mem/obj_pool.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
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

        int recv(int fd, void* buf, size_t len, size_t* nRead, u64 deadlineUs) override;
        int send(int fd, const void* buf, size_t len, size_t* nWritten, u64 deadlineUs) override;
        int writev(int fd, iovec* iov, size_t iovcnt, size_t* nWritten, u64 deadlineUs) override;
        int accept(int fd, sockaddr* addr, u32* addrLen, int* newFd, u64 deadlineUs) override;
        int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) override;

        int pread(int fd, void* buf, size_t len, off_t offset, size_t* nRead) override;
        int pwrite(int fd, const void* buf, size_t len, off_t offset, size_t* nWritten) override;
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

int PollIoReactor::recv(int fd, void* buf, size_t len, size_t* nRead, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::recv(fd, buf, len, 0);

        if (n >= 0) {
            *nRead = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor_->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::send(int fd, const void* buf, size_t len, size_t* nWritten, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::send(fd, buf, len, MSG_NOSIGNAL);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor_->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::writev(int fd, iovec* iov, size_t iovcnt, size_t* nWritten, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::writev(fd, iov, iovcnt);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor_->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::accept(int fd, sockaddr* addr, u32* addrLen, int* newFd, u64 deadlineUs) {
    for (;;) {
        socklen_t slen = addrLen ? (socklen_t)*addrLen : 0;

#ifdef __linux__
        int r = ::accept4(fd, addr, addrLen ? &slen : nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
        int r = ::accept(fd, addr, addrLen ? &slen : nullptr);
#endif

        if (r >= 0) {
            if (addrLen) {
                *addrLen = (u32)slen;
            }

            *newFd = r;

            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor_->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    if (int r = ::connect(fd, addr, addrLen); r == 0) {
        return 0;
    } else if (errno != EINPROGRESS) {
        return errno;
    }

    reactor_->poll({fd, PollFlag::Out}, deadlineUs);

    int err = 0;
    socklen_t len = sizeof(err);

    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        return errno;
    }

    return err;
}

int PollIoReactor::pread(int fd, void* buf, size_t len, off_t offset, size_t* nRead) {
    int result = 0;

    exec_->offload(offload_, [&] {
        ssize_t n = ::pread(fd, buf, len, offset);

        if (n < 0) {
            result = errno;
        } else {
            *nRead = (size_t)n;
        }
    });

    return result;
}

int PollIoReactor::pwrite(int fd, const void* buf, size_t len, off_t offset, size_t* nWritten) {
    int result = 0;

    exec_->offload(offload_, [&] {
        ssize_t n = ::pwrite(fd, buf, len, offset);

        if (n < 0) {
            result = errno;
        } else {
            *nWritten = (size_t)n;
        }
    });

    return result;
}

int PollIoReactor::fsync(int fd) {
    int result = 0;

    exec_->offload(offload_, [&] {
        if (::fsync(fd) < 0) {
            result = errno;
        }
    });

    return result;
}

int PollIoReactor::fdatasync(int fd) {
    int result = 0;

    // clang-format off
    exec_->offload(offload_, [&] {
#if defined(__APPLE__)
        if (::fcntl(fd, F_FULLFSYNC) < 0) {
            result = errno;
        }
#else
        if (::fdatasync(fd) < 0) {
            result = errno;
        }
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
