#include "in_fd_coro.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/thr/coro.h>
#include <std/thr/io_reactor.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

using namespace stl;

CoroFDInput::CoroFDInput(FD& _fd, CoroExecutor* _exec) noexcept
    : fd(&_fd)
    , exec(_exec)
    , offset(0)
{
}

CoroFDInput::~CoroFDInput() noexcept {
}

size_t CoroFDInput::readImpl(void* data, size_t len) {
    size_t n = 0;

    if (int r = exec->io()->pread(fd->get(), &n, data, len, offset)) {
        Errno(r).raise(StringBuilder() << StringView(u8"pread() failed"));
    }

    offset += n;

    return n;
}

size_t CoroFDInput::hintImpl() const noexcept {
    return 1 << 14;
}
