#include "in_fd_coro.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/thr/coro.h>
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
    auto n = exec->pread(fd->get(), data, len, offset);

    if (n < 0) {
        Errno((int)-n).raise(StringBuilder() << StringView(u8"pread() failed"));
    }

    offset += n;

    return n;
}

size_t CoroFDInput::hintImpl() const noexcept {
    return 1 << 14;
}
