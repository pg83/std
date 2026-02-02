#pragma once

#include "h_map.h"
#include "h_table.h"

namespace Std {
    template <typename T>
    using IntMap = HashMap<T, u64, HashTable>;
}
