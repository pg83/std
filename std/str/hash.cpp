#include "hash.h"

#if __has_include(<rapidhash.h>)
    #include <rapidhash.h>
#else
    #include <std/rng/split_mix_64.h>
#endif

using namespace stl;

namespace {
    static u32 xorShift(u64 v) noexcept {
        const u32 xorshifted = ((v >> 18u) ^ v) >> 27u;
        const u32 rot = v >> 59u;

        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

#if !__has_include(<rapidhash.h>)
    static u64 fnv1a(const void* data, size_t len) noexcept {
        u64 h = 14695981039346656037ull;
        auto p = (const u8*)data;

        for (size_t i = 0; i < len; ++i) {
            h ^= p[i];
            h *= 1099511628211ull;
        }

        return h;
    }
#endif
}

u32 stl::shash32(const void* data, size_t len) noexcept {
    return xorShift(shash64(data, len));
}

u64 stl::shash64(const void* data, size_t len) noexcept {
#if __has_include(<rapidhash.h>)
    return rapidhash(data, len);
#else
    return splitMix64(fnv1a(data, len));
#endif
}
