#pragma once

#include <std/sys/types.h>

namespace stl {
    u64 splitMix64(u64 x);
    u64 nextSplitMix64(u64* x);
}
