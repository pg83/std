#pragma once

#include <std/sys/types.h>

namespace stl {
    void* formatU64Base10(u64 v, void* buf);
    void* formatI64Base10(i64 v, void* buf);
    void* formatLongDouble(long double v, void* buf);
}
