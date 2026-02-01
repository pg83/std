#include "builder.h"

using namespace Std;

void StringBuilder::writeImpl(const void* ptr, size_t len) {
    append(ptr, len);
}

void* StringBuilder::imbueImpl(size_t len) {
    growDelta(len);

    return mutCurrent();
}

void StringBuilder::bumpImpl(const void* ptr) noexcept {
    seekAbsolute(ptr);
}

size_t StringBuilder::hintImpl() const noexcept {
    return size_t(-1);
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
