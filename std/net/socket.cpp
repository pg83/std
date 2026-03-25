#include "socket.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/thr/poller.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0
#define STD_SOCK_EMULATE_NB 1
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#define STD_SOCK_EMULATE_CE 1
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace stl;

namespace {
    void setSockFlags([[maybe_unused]] int fd) {
#ifdef STD_SOCK_EMULATE_NB
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
#endif

#ifdef STD_SOCK_EMULATE_CE
        ::fcntl(fd, F_SETFD, ::fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef SO_NOSIGPIPE
        int one = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    }
}

TcpSocket::TcpSocket() noexcept
    : fd(-1)
    , exec(nullptr)
{
}

TcpSocket::TcpSocket(CoroExecutor* exec) noexcept
    : fd(-1)
    , exec(exec)
{
}

TcpSocket::TcpSocket(int fd, CoroExecutor* exec) noexcept
    : fd(fd)
    , exec(exec)
{
}

int TcpSocket::socket(int domain, int type, int protocol) {
    if (fd = ::socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol); fd < 0) {
        return -errno;
    }

    setSockFlags(fd);

    return 0;
}

void TcpSocket::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

void TcpSocket::shutdown(int how) {
    ::shutdown(fd, how);
}

int TcpSocket::setReuseAddr(bool on) {
    int opt = on ? 1 : 0;

    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        return -errno;
    }

    return 0;
}

int TcpSocket::setNoDelay(bool on) {
    int opt = on ? 1 : 0;

    if (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        return -errno;
    }

    return 0;
}

int TcpSocket::bind(const sockaddr* addr, u32 addrLen) {
    if (int r = ::bind(fd, addr, addrLen); r < 0) {
        return -errno;
    }

    return 0;
}

int TcpSocket::listen(int backlog) {
    if (int r = ::listen(fd, backlog); r < 0) {
        return -errno;
    }

    return 0;
}

int TcpSocket::connect(const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    if (int r = socket(addr->sa_family, SOCK_STREAM, 0); r < 0) {
        return r;
    }

    if (int r = ::connect(fd, addr, addrLen); r == 0) {
        return 0;
    } else if (errno != EINPROGRESS) {
        return -errno;
    }

    exec->poll(fd, PollFlag::Out, deadlineUs);

    int err = 0;
    socklen_t len = sizeof(err);

    if (int r = ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len); r < 0) {
        return -errno;
    }

    if (err) {
        return -err;
    }

    return 0;
}

int TcpSocket::connectTout(const sockaddr* addr, u32 addrLen, u64 timeoutUs) {
    return connect(addr, addrLen, monotonicNowUs() + timeoutUs);
}

int TcpSocket::connectInf(const sockaddr* addr, u32 addrLen) {
    return connect(addr, addrLen, UINT64_MAX);
}

int TcpSocket::accept(ScopedFD& out, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    exec->poll(fd, PollFlag::In, deadlineUs);

    socklen_t slen = addrLen ? (socklen_t)*addrLen : 0;

#ifdef __linux__
    int newFd = ::accept4(fd, addr, addrLen ? &slen : nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    int newFd = ::accept(fd, addr, addrLen ? &slen : nullptr);
#endif

    if (newFd < 0) {
        return -errno;
    }

    setSockFlags(newFd);

    if (addrLen) {
        *addrLen = (u32)slen;
    }

    ScopedFD tmp(newFd);
    out.xchg(tmp);

    return 0;
}

int TcpSocket::acceptTout(ScopedFD& out, sockaddr* addr, u32* addrLen, u64 timeoutUs) {
    return accept(out, addr, addrLen, monotonicNowUs() + timeoutUs);
}

int TcpSocket::acceptInf(ScopedFD& out, sockaddr* addr, u32* addrLen) {
    return accept(out, addr, addrLen, UINT64_MAX);
}

int TcpSocket::read(size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        if (ssize_t n = ::read(fd, buf, len); n > 0) {
            *nRead = (size_t)n;
            return 0;
        } else if (n == 0) {
            *nRead = 0;
            return 0;
        } else if (errno != EAGAIN) {
            return -errno;
        }

        exec->poll(fd, PollFlag::In, deadlineUs);
    }
}

int TcpSocket::readTout(size_t* nRead, void* buf, size_t len, u64 timeoutUs) {
    return read(nRead, buf, len, monotonicNowUs() + timeoutUs);
}

int TcpSocket::readInf(size_t* nRead, void* buf, size_t len) {
    return read(nRead, buf, len, UINT64_MAX);
}

int TcpSocket::write(size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        if (ssize_t n = ::send(fd, buf, len, MSG_NOSIGNAL); n > 0) {
            *nWritten = (size_t)n;
            return 0;
        } else if (errno != EAGAIN) {
            return -errno;
        }

        exec->poll(fd, PollFlag::Out, deadlineUs);
    }
}

int TcpSocket::writeTout(size_t* nWritten, const void* buf, size_t len, u64 timeoutUs) {
    return write(nWritten, buf, len, monotonicNowUs() + timeoutUs);
}

int TcpSocket::writeInf(size_t* nWritten, const void* buf, size_t len) {
    return write(nWritten, buf, len, UINT64_MAX);
}

int TcpSocket::writev(size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    for (;;) {
        if (ssize_t n = ::writev(fd, iov, iovcnt); n > 0) {
            *nWritten = (size_t)n;
            return 0;
        } else if (errno != EAGAIN) {
            return -errno;
        }

        exec->poll(fd, PollFlag::Out, deadlineUs);
    }
}

int TcpSocket::writevInf(size_t* nWritten, iovec* iov, size_t iovcnt) {
    return writev(nWritten, iov, iovcnt, UINT64_MAX);
}
