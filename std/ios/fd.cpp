#include "fd.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace Std;

FDOutput::~FDOutput() noexcept {
}

size_t FDOutput::writeImpl(const void* data, size_t len) {
    auto res = ::write(fd, data, len);

    if (res < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"write failed"));
    }

    return res;
}

void FDOutput::flushImpl() {
    if (fsync(fd) < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"fsync failed"));
    }
}

void FDOutput::finishImpl() {
    fd = -1;
}

size_t FDOutput::hintImpl() const noexcept {
    // https://man7.org/linux/man-pages/man2/write.2.html
    return 0x7ffff000;
}

static inline int writev_all(int fd, iovec* iov, size_t iovcnt) {
    while (iovcnt > 0) {
        ssize_t written = writev(fd, iov, iovcnt);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        while (written >= iov->iov_len) {
            written -= iov->iov_len;
            iov++;
            iovcnt--;
        }

        if (written > 0) {
            iov->iov_base = (char*)iov->iov_base + written;
            iov->iov_len -= written;
        }
    }

    return 0;
}

void FDOutput::writeVImpl(iovec* parts, size_t count) {
    if (writev_all(fd, parts, count) < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"writev failed"));
    }
}
