#pragma once

#include "xchg.h"

#include <std/sys/types.h>

namespace Std {
    template <typename RNG, typename I>
    inline void shuffle(RNG& r, I b, I e) noexcept {
        for (auto n = e - b; n > 1; --n) {
            xchg(b[n - 1], b[r.uniformUnbiased((u32)n)]);
        }
    }
}
