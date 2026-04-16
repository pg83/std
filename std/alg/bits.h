#pragma once

#include <std/sys/types.h>

namespace stl {
    template <class T>
    inline T clp2(T v) noexcept {
        if constexpr (sizeof(T) == 4) {
            return v <= 1 ? T(1) : T(1) << (32 - __builtin_clz(v - 1));
        } else if constexpr (sizeof(T) == 8) {
            return v <= 1 ? T(1) : T(1) << (64 - __builtin_clzll(v - 1));
        } else {
            static_assert(false, "shit happen");
        }
    }
}
