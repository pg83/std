#include "builder.h"

using namespace stl;

size_t StringBuilder::writeImpl(const void* ptr, size_t len) {
    return (append(ptr, len), len);
}

void* StringBuilder::imbueImpl(size_t* len) {
    return imbueMe(len);
}

void StringBuilder::commitImpl(size_t len) {
    seekRelative(len);
}

StringBuilder::StringBuilder() {
}

StringBuilder::StringBuilder(Buffer&& buf) {
    buf.xchg(*this);
}

StringBuilder::StringBuilder(size_t reserve)
    : Buffer(reserve)
{
}

StringBuilder::~StringBuilder() {
}
