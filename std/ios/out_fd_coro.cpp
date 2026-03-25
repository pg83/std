#include "out_fd_coro.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/thr/coro.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

using namespace stl;

CoroFDOutput::CoroFDOutput(FD& _fd, CoroExecutor* _exec) noexcept
    : fd(&_fd)
    , exec(_exec)
    , offset(0)
{
}

CoroFDOutput::~CoroFDOutput() noexcept {
}

size_t CoroFDOutput::writeImpl(const void* data, size_t len) {
    auto n = exec->pwrite(fd->get(), data, len, offset);

    if (n < 0) {
        Errno((int)-n).raise(StringBuilder() << StringView(u8"pwrite() failed"));
    }

    offset += n;

    return n;
}

size_t CoroFDOutput::hintImpl() const noexcept {
    return 1 << 14;
}

void CoroFDOutput::sync() {
    auto n = exec->fsync(fd->get());

    if (n < 0) {
        Errno((int)-n).raise(StringBuilder() << StringView(u8"fsync() failed"));
    }
}

void CoroFDOutput::dataSync() {
    auto n = exec->fdatasync(fd->get());

    if (n < 0) {
        Errno((int)-n).raise(StringBuilder() << StringView(u8"fdatasync() failed"));
    }
}
