#include "i_map.h"

#include <std/rng/split_mix_64.h>

using namespace stl;

u64 IntHasher::hash(u64 k) noexcept {
    return k;
    return splitMix64(k);
}
