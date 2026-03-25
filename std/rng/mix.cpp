#include "mix.h"
#include "split_mix_64.h"

using namespace stl;

namespace {
    u64 mixPtr(const void* p) noexcept {
        return splitMix64((uintptr_t)p);
    }
}

u64 stl::mix(const void* a) noexcept {
    return mixPtr(a);
}

u64 stl::mix(const void* a, const void* b) noexcept {
    return splitMix64((uintptr_t)a ^ mixPtr(b));
}

u64 stl::mix(const void* a, const void* b, const void* c) noexcept {
    return splitMix64((uintptr_t)a ^ splitMix64((uintptr_t)b ^ mixPtr(c)));
}
