#pragma once

#include <std/sys/types.h>

namespace stl {
    template <class T>
    inline T clp2(T v) noexcept {
        static_assert(T(-1) > T(0));
        static_assert(sizeof(T) == 4 || sizeof(T) == 8);

        if constexpr (sizeof(T) <= 4) {
            return v <= 1 ? T(1) : T(1) << (32 - __builtin_clz(v - 1));
        } else {
            return v <= 1 ? T(1) : T(1) << (64 - __builtin_clzll(v - 1));
        }
    }
}
