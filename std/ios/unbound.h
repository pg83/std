#pragma once

#include "outable.h"

#include <std/sys/types.h>

namespace Std {
    struct UnboundBuffer {
        void* ptr;

        inline size_t distance(UnboundBuffer e) const noexcept {
            return (const u8*)e.ptr - (const u8*)ptr;
        }
    };

    template <typename T>
    inline UnboundBuffer operator<<(UnboundBuffer out, const T& t) {
        output<UnboundBuffer, T>(out, t);

        return out;
    }
}
