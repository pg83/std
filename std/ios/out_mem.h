#pragma once

#include "out_zc.h"

namespace Std {
    class MemoryOutput: public ZeroCopyOutput {
        void* imbueImpl(size_t* avail) override;
        void commitImpl(size_t len) noexcept override;

    public:
        void* ptr;

        inline MemoryOutput(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }
    };
}
