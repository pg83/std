#pragma once

#include "h_map.h"
#include "s_table.h"

namespace Std {
    template <typename T>
    using SymbolMap = HashMap<T, StringView, SymbolTable>;
}
