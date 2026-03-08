#include "out_fd.h"

#include <std/sys/fd.h>

using namespace stl;

FDOutput::~FDOutput() {
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

size_t FDRegular::hintImpl() const {
    return 1 << 14;
}

FDRegular::FDRegular(FD& fd)
    : FDOutput(fd)
{
}

size_t FDCharacter::hintImpl() const {
    return 1 << 10;
}

FDCharacter::FDCharacter(FD& fd)
    : FDOutput(fd)
{
}

size_t FDPipe::hintImpl() const {
    return 1 << 12;
}

FDPipe::FDPipe(FD& fd)
    : FDOutput(fd)
{
}
