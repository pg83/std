#include "io_classic.h"

#include "coro.h"
#include "pool.h"
#include "cond_var.h"
#include "poll_fd.h"
#include "io_reactor.h"
#include "reactor_poll.h"

#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>
#include <std/rng/split_mix_64.h>

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
    struct PollIoReactor: public IoReactor, public ThreadPoolHooks {
        Vector<ReactorIface*> reactors_;
        CoroExecutor* exec_;
        ThreadPool* offload_;

        PollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors);

        ReactorIface* reactor(int fd) noexcept {
            return reactors_[splitMix64(fd) % reactors_.length()];
        }

        ThreadPoolHooks* hooks() override {
            return this;
        }

        CondVarIface* createCondVar(size_t) override;

        PollGroup* createPollGroup(ObjPool* pool, const PollFD* fds, size_t count) override;

        int recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) override;
        int recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) override;
        int send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) override;
        int sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) override;
        int writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) override;
        int accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) override;
        int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) override;

        int pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) override;
        int pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) override;
        int fsync(int fd) override;
        int fdatasync(int fd) override;

        u32 poll(PollFD pfd, u64 deadlineUs) override;
        void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) override;
        void sleep(u64 deadlineUs) override;
    };
}

PollIoReactor::PollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors)
    : exec_(exec)
    , offload_(ThreadPool::simple(pool, reactors))
{
    for (size_t i = 0; i < reactors; ++i) {
        reactors_.pushBack(ReactorIface::create(exec, pool));
    }
}

int PollIoReactor::recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::recv(fd, buf, len, 0);

        if (n >= 0) {
            *nRead = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    for (;;) {
        socklen_t slen = addrLen ? (socklen_t)*addrLen : 0;
        ssize_t n = ::recvfrom(fd, buf, len, 0, addr, addrLen ? &slen : nullptr);

        if (n >= 0) {
            *nRead = (size_t)n;

            if (addrLen) {
                *addrLen = (u32)slen;
            }

            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::send(fd, buf, len, MSG_NOSIGNAL);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::sendto(fd, buf, len, MSG_NOSIGNAL, addr, addrLen);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::writev(fd, iov, iovcnt);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
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

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
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

    reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs);

    int err = 0;
    socklen_t len = sizeof(err);

    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        return errno;
    }

    return err;
}

int PollIoReactor::pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) {
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

int PollIoReactor::pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) {
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
    return reactor(pfd.fd)->poll(pfd, deadlineUs);
}

void PollIoReactor::poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) {
    reactor(ReactorIface::pollGroupFd(g))->poll(g, visitor, deadlineUs);
}

void PollIoReactor::sleep(u64 deadlineUs) {
    poll({-1, 0}, deadlineUs);
}

PollGroup* PollIoReactor::createPollGroup(ObjPool* pool, const PollFD* fds, size_t count) {
    return ReactorIface::createPollGroup(pool, fds, count);
}

CondVarIface* PollIoReactor::createCondVar(size_t) {
    return CondVar::createDefault();
}

IoReactor* stl::createPollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors) {
    return pool->make<PollIoReactor>(pool, exec, reactors);
}
