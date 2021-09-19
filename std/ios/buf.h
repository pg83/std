#pragma once

#include "output.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace Std {
    class OutBuf: public ZeroCopyOutput {
        Output* out_;
        Buffer buf_;

    public:
        ~OutBuf();

        OutBuf(Output& out) noexcept;

        inline OutBuf(OutBuf&& buf) noexcept
            : OutBuf()
        {
            buf.xchg(*this);
        }

        OutBuf(const OutBuf&) = delete;

        void xchg(OutBuf& buf) noexcept;

        inline Output& stream() noexcept {
            return *out_;
        }

    private:
        OutBuf() noexcept;

        // state
        void flushImpl() override;
        void finishImpl() override;

        // classic
        void writeImpl(const void* ptr, size_t len) override;

        // zero-copy
        size_t imbueImpl(void** ptr) noexcept override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
    };
}
