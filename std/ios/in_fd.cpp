#include "in_fd.h"

#include <std/sys/fd.h>

using namespace stl;

size_t FDInput::readImpl(void* data, size_t len) {
    return fd->read(data, len);
}

FDInput::~FDInput() {
}

size_t FDInput::hintImpl() const {
    return 1 << 14;
}
