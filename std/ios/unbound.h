#pragma once

#include "outable.h"

namespace Std {
    struct UnboundBuffer {
        void* ptr;
    };

    template <typename T>
    inline UnboundBuffer operator<<(UnboundBuffer out, const T& t) {
        output<UnboundBuffer, T>(out, t);

        return out;
    }
}
