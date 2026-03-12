#include "fd.h"

#include <std/alg/xchg.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/alg/minmax.h>
#include <std/str/builder.h>
#include <std/alg/exchange.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace stl;

size_t FD::read(void* data, size_t len) {
    if (auto res = ::read(fd, data, min(len, (size_t)(0x7ffff000 - 1))); res >= 0) {
        return res;
    }

    if (errno == EFAULT && len > 1024) {
        return read(data, len / 2);
    }

    Errno().raise(StringBuilder() << StringView(u8"read() failed"));
}

size_t FD::readNB(void* data, size_t len) {
    size_t total = 0;
    auto* buf = (char*)data;

    while (total < len) {
        struct pollfd pfd = {fd, POLLIN, 0};

        if (::poll(&pfd, 1, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }

            Errno().raise(StringBuilder() << StringView(u8"poll() failed"));
        }

        auto res = ::read(fd, buf + total, len - total);

        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }

            Errno().raise(StringBuilder() << StringView(u8"readNB() failed"));
        }

        total += res;
    }

    return total;
}

size_t FD::tryReadNB(void* data, size_t len) {
    auto res = ::read(fd, data, len);

    if (res < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        Errno().raise(StringBuilder() << StringView(u8"tryReadNB() failed"));
    }

    return res;
}

size_t FD::write(const void* data, size_t len) {
    auto res = ::write(fd, data, len);

    if (res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"write() failed"));
    }

    return res;
}

size_t FD::writeV(iovec* parts, size_t count) {
    auto res = writev(fd, parts, count);

    if (res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"writev() failed"));
    }

    return res;
}

void FD::setNonBlocking() {
    if (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        Errno().raise(StringBuilder() << StringView(u8"fcntl(O_NONBLOCK) failed"));
    }
}

void FD::fsync() {
    if (::fsync(fd) < 0) {
        Errno().raise(StringBuilder() << StringView(u8"fsync() failed"));
    }
}

void FD::close() {
    if (fd < 0) {
        return;
    }

    if (::close(exchange(fd, -1)) < 0) {
        Errno().raise(StringBuilder() << StringView(u8"close() failed"));
    }
}

void FD::xchg(FD& other) noexcept {
    ::stl::xchg(fd, other.fd);
}

ScopedFD::~ScopedFD() noexcept(false) {
    close();
}

void stl::createPipeFD(ScopedFD& in, ScopedFD& out) {
    int fd[2];

    if (auto res = pipe(fd); res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"pipe() failed"));
    }

    ScopedFD(fd[0]).xchg(in);
    ScopedFD(fd[1]).xchg(out);
}
