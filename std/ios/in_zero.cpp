#include "in_zero.h"

using namespace stl;

size_t ZeroInput::readImpl(void* data, size_t len) {
    (void)data;
    (void)len;
    return 0;
}

size_t ZeroInput::nextImpl(const void** chunk) {
    *chunk = "";

    return 0;
}

void ZeroInput::commitImpl(size_t len) noexcept {
    (void)len;
}
