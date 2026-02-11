#include "builder.h"

using namespace Std;

size_t StringBuilder::writeImpl(const void* ptr, size_t len) {
    return (append(ptr, len), len);
}

void* StringBuilder::imbueImpl(size_t* len) {
    return imbueMe(len);
}

void StringBuilder::commitImpl(size_t len) noexcept {
    seekRelative(len);
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
