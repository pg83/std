#include "fd.h"

#include <std/sys/crt.h>
#include <std/str/view.h>

#include <fcntl.h>
#include <unistd.h>
#include <alloca.h>
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

void FDOutput::writeVImpl(const StringView* parts, size_t count) {
    iovec* io = (iovec*)alloca(count * sizeof(iovec));

    memZero(io, io + count);

    for (size_t i = 0; i < count; ++i) {
        io[i].iov_len = parts[i].length();
        io[i].iov_base = (void*)parts[i].data();
    }

    writev(fd, io, count);
}
