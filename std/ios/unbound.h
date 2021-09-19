#pragma once

#include "outable.h"

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

        return static_cast<UnboundBuffer&&>(out);
    }
}
