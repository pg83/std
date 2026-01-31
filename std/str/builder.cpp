#include "builder.h"

using namespace Std;

void StringBuilder::writeImpl(const void* ptr, size_t len) {
    append(ptr, len);
}

void* StringBuilder::imbueImpl(size_t len) {
    grow(len);

    return mutCurrent();
}

void StringBuilder::bumpImpl(const void* ptr) noexcept {
    seekAbsolute(ptr);
}

size_t StringBuilder::hintImpl() const noexcept {
    return size_t(-1);
}

StringBuilder::StringBuilder() {
}

StringBuilder::StringBuilder(size_t reserve) {
    grow(reserve);
}

StringBuilder::~StringBuilder() noexcept {
}
