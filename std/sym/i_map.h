#pragma once

#include "h_map.h"
#include "h_table.h"

namespace Std {
    struct IntHasher {
        static inline u64 hash(u64 k) noexcept {
            return k;
        }
    };

    template <typename T>
    using IntMap = HashMap<T, u64, IntHasher>;
}
