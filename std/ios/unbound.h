#pragma once

#include "outable.h"

#include <std/typ/support.h>

namespace Std {
    struct UnboundBuffer {
        void* ptr;

        inline UnboundBuffer(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }
    };

    template <typename T>
    inline UnboundBuffer&& operator<<(UnboundBuffer&& out, const T& t) {
        output<UnboundBuffer, T>(out, t);

        return forward<UnboundBuffer>(out);
    }
}
