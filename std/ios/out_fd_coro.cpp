#include "out_fd_coro.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/thr/coro.h>
#include <std/thr/io_reactor.h>
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
    size_t n = 0;

    if (int r = exec->io()->pwrite(fd->get(), &n, data, len, offset)) {
        Errno(r).raise(StringBuilder() << StringView(u8"pwrite() failed"));
    }

    offset += n;

    return n;
}

size_t CoroFDOutput::hintImpl() const noexcept {
    return 1 << 14;
}

void CoroFDOutput::sync() {
    if (int r = exec->io()->fsync(fd->get())) {
        Errno(r).raise(StringBuilder() << StringView(u8"fsync() failed"));
    }
}

void CoroFDOutput::dataSync() {
    if (int r = exec->io()->fdatasync(fd->get())) {
        Errno(r).raise(StringBuilder() << StringView(u8"fdatasync() failed"));
    }
}
