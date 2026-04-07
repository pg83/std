#include "out_null.h"

using namespace stl;

NullOutput::~NullOutput() noexcept {
}

size_t NullOutput::writeImpl(const void* data, size_t len) {
    (void)data;

    return len;
}
