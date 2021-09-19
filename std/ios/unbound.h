#pragma once

#include "outable.h"

namespace Std {
    struct UnboundBuffer: public OutAble {
        void* ptr;

        inline UnboundBuffer(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }
    };
}
