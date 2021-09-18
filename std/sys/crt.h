#pragma once

#include "types.h"

namespace Std {
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr) noexcept;
    int memCmp(const void* l, const void* r, size_t len) noexcept;
}
