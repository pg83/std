#pragma once

#include <std/sys/types.h>

namespace stl {
    class PCG32 {
        u64 state_;
        u64 seq_;

    public:
        PCG32(u64 seq);
        PCG32(const void* seq);
        PCG32(u64 state, u64 seq);

        u32 nextU32();

        u32 uniformBiased(u32 n) {
            // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
            return ((u64)nextU32() * (u64)n) >> 32;
        }

        u32 uniformUnbiased(u32 n);
    };
};
