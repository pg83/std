#pragma once

#include "h_table.h"

#include <std/str/view.h>

namespace Std {
    struct SymbolTable: public HashTable {
        static inline u64 hash(StringView k) noexcept {
            return k.hash64();
        }
    };
}
