#pragma once

#include "types.h"

namespace stl {
    // allocator
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr);

    // string ops
    void memZero(void* from, void* to);
    int memCmp(const void* l, const void* r, size_t len);
    void* memCpy(void* to, const void* from, size_t len);
    size_t strLen(const u8* s);
}
