#pragma once

#include "in_zc.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace stl {
    class InBuf: public ZeroCopyInput {
        Input* in_;
        Buffer buf;
        size_t pos;

        InBuf() noexcept;

        size_t hintImpl() const noexcept override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

    public:
        ~InBuf() override;

        InBuf(Input& in) noexcept;
        InBuf(Input& in, size_t chunkSize) noexcept;

        InBuf(InBuf&& buf) noexcept
            : InBuf()
        {
            buf.xchg(*this);
        }

        InBuf(const InBuf&) = delete;

        void xchg(InBuf& buf) noexcept;

        Input& stream() noexcept {
            return *in_;
        }
    };
}
