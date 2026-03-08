#pragma once

#include <std/sys/types.h>

namespace stl {
    inline u32 clp2(u32 v) noexcept {
        return v <= 1 ? 1u : 1u << (32 - __builtin_clz(v - 1));
    }

    inline u64 clp2(u64 v) noexcept {
        return v <= 1 ? 1ull : 1ull << (64 - __builtin_clzll(v - 1));
    }
}
