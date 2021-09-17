#pragma once

#include "types.h"

namespace Std {
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr) noexcept;
}
