#include "pcg.h"
#include "split_mix_64.h"

#include <std/alg/exchange.h>

using namespace Std;

namespace {
    static inline u32 xorShift(u64 v) noexcept {
        const u32 xorshifted = ((v >> 18u) ^ v) >> 27u;
        const u32 rot = v >> 59u;

        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }
}

PCG32::PCG32(u64 seq) noexcept
    : PCG32(nextSplitMix64((size_t)this), seq)
{
}

PCG32::PCG32(const void* seq) noexcept
    : PCG32(nextSplitMix64((size_t)seq))
{
}

PCG32::PCG32(u64 state, u64 seq) noexcept
    : state_(0)
    , seq_(2ULL * seq + 1ULL)
{
    nextU32();
    state_ += state;
    nextU32();
}

u32 PCG32::nextU32() noexcept {
    return xorShift(exchange(state_, state_ * 6364136223846793005ULL + seq_));
}
