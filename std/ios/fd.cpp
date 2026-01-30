#include "fd.h"

#include <std/str/view.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace Std;

FDOutput::~FDOutput() noexcept {
}

void FDOutput::writeImpl(const void* data, size_t len) {
    ::write(fd, data, len);
}

void FDOutput::flushImpl() {
    fsync(fd);
}

void FDOutput::finishImpl() {
    fd = -1;
}

size_t FDOutput::hintImpl() const noexcept {
    // https://man7.org/linux/man-pages/man2/write.2.html
    return 0x7ffff000;
}

static inline int writev_all(int fd, iovec* iov, int iovcnt) {
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
    writev_all(fd, parts, count);
}
