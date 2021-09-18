#pragma once

#include <std/sys/types.h>

namespace Std {
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr) noexcept;
}
