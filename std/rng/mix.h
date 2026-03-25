#pragma once

#include <std/sys/types.h>

namespace stl {
    u64 mix(const void* a) noexcept;
    u64 mix(const void* a, const void* b) noexcept;
    u64 mix(const void* a, const void* b, const void* c) noexcept;
}
