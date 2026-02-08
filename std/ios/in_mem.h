#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace Std {
    class MemoryInput: public Input {
        const u8* b;
        const u8* e;

        size_t readImpl(void* data, size_t len) override;

    public:
        inline MemoryInput(const void* data, size_t len) noexcept
            : b((const u8*)data)
            , e(b + len)
        {
        }
    };
}
