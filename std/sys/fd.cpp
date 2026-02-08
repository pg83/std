#include "fd.h"

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
    if (::close(exchange(fd, -1)) < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"close() failed"));
    }
}
