#pragma once

#include <std/sys/types.h>

namespace Std {
    u32 shash32(const void* data, size_t len);
    u64 shash64(const void* data, size_t len);
}
