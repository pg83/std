#pragma once

#include "zc_out.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace Std {
    class OutBuf: public ZeroCopyOutput {
        Output* out_;
        Buffer buf_;
        size_t chunk;

        OutBuf() noexcept;

        size_t writeMultipart(const void* ptr);
        size_t writeDirect(const void* ptr, size_t len);

        // state
        void flushImpl() override;
        void finishImpl() override;

        // classic
        size_t writeImpl(const void* ptr, size_t len) override;
        size_t hintImpl() const noexcept override;

        // zero-copy
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;

    public:
        ~OutBuf() override;

        OutBuf(Output& out) noexcept;
        OutBuf(Output& out, size_t chunkSize) noexcept;

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
    };
}
