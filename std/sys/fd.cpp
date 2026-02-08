#include "fd.h"

#include <std/alg/xchg.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>
#include <std/alg/exchange.h>

#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace Std;

size_t FD::read(void* data, size_t len) {
    auto res = ::read(fd, data, len);

    if (res < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"read() failed"));
    }

    return res;
}

size_t FD::write(const void* data, size_t len) {
    auto res = ::write(fd, data, len);

    if (res < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"write() failed"));
    }

    return res;
}

size_t FD::writeV(iovec* parts, size_t count) {
    auto res = writev(fd, parts, count);

    if (res < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"writev() failed"));
    }

    return res;
}

void FD::fsync() {
    if (::fsync(fd) < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"fsync() failed"));
    }
}

void FD::close() {
    if (fd < 0) {
        return;
    }

    if (::close(exchange(fd, -1)) < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"close() failed"));
    }
}

void FD::xchg(FD& other) noexcept {
    ::Std::xchg(fd, other.fd);
}

void Std::createPipeFD(ScopedFD& in, ScopedFD& out) {
    int fd[2];

    if (auto res = pipe(fd); res < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"pipe() failed"));
    }

    ScopedFD(fd[0]).xchg(in);
    ScopedFD(fd[1]).xchg(out);
}
