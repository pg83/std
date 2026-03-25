#include "mix.h"
#include "split_mix_64.h"

using namespace stl;

u64 stl::mix(const void* a) noexcept {
    return splitMix64((uintptr_t)a);
}

u64 stl::mix(const void* a, const void* b) noexcept {
    return splitMix64((uintptr_t)a ^ mix(b));
}

u64 stl::mix(const void* a, const void* b, const void* c) noexcept {
    return splitMix64((uintptr_t)a ^ mix(b, c));
}
