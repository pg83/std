#include "fd.h"

#include <std/sys/fd.h>

using namespace Std;

FDOutput::~FDOutput() noexcept {
}

size_t FDOutput::writeImpl(const void* data, size_t len) {
    return fd->write(data, len);
}

void FDOutput::finishImpl() {
    fd = nullptr;
}

size_t FDOutput::writeVImpl(iovec* parts, size_t count) {
    return fd->writeV(parts, count);
}

void FDRegular::flushImpl() {
    fd->fsync();
}

size_t FDRegular::hintImpl() const noexcept {
    return 1 << 16;
}

FDRegular::FDRegular(FD& fd) noexcept
    : FDOutput(fd)
{
}

size_t FDCharacter::hintImpl() const noexcept {
    return 1 << 10;
}

FDCharacter::FDCharacter(FD& fd) noexcept
    : FDOutput(fd)
{
}

size_t FDPipe::hintImpl() const noexcept {
    return 1 << 12;
}

FDPipe::FDPipe(FD& fd) noexcept
    : FDOutput(fd)
{
}
