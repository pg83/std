#pragma once

#include <std/sys/types.h>

namespace Std {
    class PCG32 {
        u64 state_;
        u64 seq_;

    public:
        PCG32(u64 state, u64 seq) noexcept;

        inline PCG32(u64 seq) noexcept
            : PCG32((size_t)this, seq)
        {
        }

        PCG32(void* seq) noexcept;

        u32 nextU32() noexcept;

        inline u32 uniformBiased(u32 n) noexcept {
            // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
            return ((u64)nextU32() * (u64)n) >> 32;
        }
    };
};
