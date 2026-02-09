#pragma once

#include "out_zc.h"

namespace Std {
    class MemoryOutput: public ZeroCopyOutput {
        size_t writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t* avail) override;
        void commitImpl(const void* ptr) noexcept override;

    public:
        void* ptr;

        inline MemoryOutput(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }
    };
}
