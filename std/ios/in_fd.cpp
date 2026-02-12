#include "in_fd.h"

#include <std/sys/fd.h>

using namespace Std;

size_t FDInput::readImpl(void* data, size_t len) {
    return fd->read(data, len);
}

FDInput::~FDInput() noexcept {
}

size_t FDInput::hintImpl() const noexcept {
    return 4096;
}
