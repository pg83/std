#include "hash.h"

#include <rapidhash.h>

using namespace Std;

namespace {
    static inline u32 xorShift(u64 v) noexcept {
        const u32 xorshifted = ((v >> 18u) ^ v) >> 27u;
        const u32 rot = v >> 59u;

        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }
}

u32 Std::shash32(const void* data, size_t len) noexcept {
    return xorShift(shash64(data, len));
}

u64 Std::shash64(const void* data, size_t len) noexcept {
    return rapidhash(data, len);
}
