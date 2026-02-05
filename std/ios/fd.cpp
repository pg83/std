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

void FDOutput::finishImpl() {
    fd = -1;
}

size_t FDOutput::writeVImpl(iovec* parts, size_t count) {
    auto res = writev(fd, parts, count);

    if (res < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"writev failed"));
    }

    return res;
}

void FDRegular::flushImpl() {
    if (fsync(fd) < 0) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"fsync failed"));
    }
}

size_t FDRegular::hintImpl() const noexcept {
    return 1 << 16;
}

FDRegular::FDRegular(int fd) noexcept
    : FDOutput(fd)
{
}

size_t FDCharacter::hintImpl() const noexcept {
    return 1 << 10;
}

FDCharacter::FDCharacter(int fd) noexcept
    : FDOutput(fd)
{
}

size_t FDPipe::hintImpl() const noexcept {
    return 1 << 12;
}

FDPipe::FDPipe(int fd) noexcept
    : FDOutput(fd)
{
}
