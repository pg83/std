#pragma once

#include "outable.h"

#include <std/sys/types.h>

namespace stl {
    struct UnboundBuffer {
        void* ptr;

        size_t distance(UnboundBuffer e) const noexcept {
            return (const u8*)e.ptr - (const u8*)ptr;
        }
    };

    template <typename T>
    UnboundBuffer operator<<(UnboundBuffer out, const T& t) {
        output<UnboundBuffer, T>(out, t);

        return out;
    }
}
