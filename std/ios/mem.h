#pragma once

#include "zc_out.h"

namespace Std {
    struct MemoryOutput: public ZeroCopyOutput {
        void* ptr;

        inline MemoryOutput(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }

        void writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
    };
}
