#pragma once

#include "h_map.h"

#include <std/str/view.h>

namespace Std {
    struct SymbolHasher {
        static inline u64 hash(StringView k) noexcept {
            return k.hash64();
        }
    };

    template <typename T>
    using SymbolMap = HashMap<T, StringView, SymbolHasher>;
}
