#pragma once

#include "h_map.h"

namespace stl {
    struct IntHasher {
        static inline u64 hash(u64 k) noexcept {
            return k;
        }
    };

    template <typename T>
    using IntMap = HashMap<T, u64, IntHasher>;
}
