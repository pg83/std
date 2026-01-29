#include "fd.h"

#include <fcntl.h>
#include <unistd.h>

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
    return 64 * 1024;
}
