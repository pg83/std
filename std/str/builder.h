#pragma once

#include <std/ios/zc_out.h>
#include <std/lib/buffer.h>

namespace Std {
    class StringBuilder: public ZeroCopyOutput, public Buffer {
        size_t writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
        size_t hintImpl() const noexcept override;

    public:
        StringBuilder() noexcept;
        StringBuilder(size_t reserve);
        StringBuilder(Buffer&& buf) noexcept;

        ~StringBuilder() noexcept override;
    };
}
