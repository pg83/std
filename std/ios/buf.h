#pragma once

#include "zc_out.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace Std {
    class OutBuf: public ZeroCopyOutput {
        Output* out_;
        Buffer buf_;

    public:
        ~OutBuf() override;

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
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
    };
}
