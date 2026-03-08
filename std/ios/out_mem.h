#pragma once

#include "out_zc.h"

namespace stl {
    class MemoryOutput: public ZeroCopyOutput {
        void* imbueImpl(size_t* avail) override;
        void commitImpl(size_t len) override;

    public:
        void* ptr;

        MemoryOutput(void* _ptr)
            : ptr(_ptr)
        {
        }
    };
}
