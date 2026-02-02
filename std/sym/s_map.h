#pragma once

#include "h_map.h"
#include "s_table.h"

#include <std/typ/support.h>
#include <std/mem/obj_pool.h>

namespace Std {
    template <typename T>
    using SymbolMap = HashMap<T, StringView, SymbolTable>;
}
