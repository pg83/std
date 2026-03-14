#pragma once

#include "h_map.h"

namespace stl {
    struct IntHasher {
        static u64 hash(u64 k) noexcept;
    };

    template <typename T>
    using IntMap = HashMap<T, u64, IntHasher>;
}
