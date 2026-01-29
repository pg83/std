#pragma once

#include "zc_out.h"

#include <std/lib/buffer.h>

namespace Std {
    class CountingOutput: public ZeroCopyOutput {
        u64 len_;
        Buffer buf_;

    public:
        CountingOutput() noexcept;

        inline auto collectedLength() const noexcept {
            return len_;
        }

        void writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
    };
}
