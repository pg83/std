#pragma once

#include "types.h"

namespace Std {
    // allocator
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr) noexcept;

    // string ops
    int memCmp(const void* l, const void* r, size_t len) noexcept;
    size_t strLen(const u8* s) noexcept;
}
