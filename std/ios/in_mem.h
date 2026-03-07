#pragma once

#include "in_zc.h"

#include <std/sys/types.h>

namespace stl {
    class MemoryInput: public ZeroCopyInput {
        const u8* b;
        const u8* e;

        void sendTo(Output& out) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

    public:
        MemoryInput(const void* data, size_t len) noexcept
            : b((const u8*)data)
            , e(b + len)
        {
        }
    };
}
