#pragma once

#include "types.h"

namespace Std {
    // allocator
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr) noexcept;

    // string ops
    void memZero(void* from, void* to) noexcept;
    int memCmp(const void* l, const void* r, size_t len) noexcept;
    void* memCpy(void* to, const void* from, size_t len) noexcept;
    size_t strLen(const u8* s) noexcept;
}
