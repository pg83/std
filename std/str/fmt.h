#pragma once

#include <std/sys/types.h>

namespace Std {
    void* formatU64Base10(u64 v, void* buf) noexcept;
    void* formatI64Base10(i64 v, void* buf) noexcept;
}
