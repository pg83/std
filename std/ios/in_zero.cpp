#include "in_zero.h"

using namespace stl;

size_t ZeroInput::nextImpl(const void** chunk) {
    (void)chunk;
    return 0;
}

void ZeroInput::commitImpl(size_t len) noexcept {
    (void)len;
}
