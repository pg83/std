#include "builder.h"

using namespace Std;

size_t StringBuilder::writeImpl(const void* ptr, size_t len) {
    return (append(ptr, len), len);
}

void* StringBuilder::imbueImpl(size_t len, size_t* avail) {
    growDelta(len);

    *avail = left();

    return mutCurrent();
}

void StringBuilder::bumpImpl(const void* ptr) noexcept {
    seekAbsolute(ptr);
}

StringBuilder::StringBuilder() noexcept {
}

StringBuilder::StringBuilder(Buffer&& buf) noexcept {
    buf.xchg(*this);
}

StringBuilder::StringBuilder(size_t reserve)
    : Buffer(reserve)
{
}

StringBuilder::~StringBuilder() noexcept {
}
