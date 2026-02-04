#pragma once

#include "zc_out.h"

namespace Std {
    class MemoryOutput: public ZeroCopyOutput {
        size_t writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
        size_t hintImpl() const noexcept override;

    public:
        void* ptr;

        inline MemoryOutput(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }
    };
}
