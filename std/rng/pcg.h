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

        u32 nextU32() noexcept;
    };
};
