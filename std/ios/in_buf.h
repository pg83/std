#pragma once

#include "in_zc.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace stl {
    class InBuf: public ZeroCopyInput {
        Input* in_;
        Buffer buf;
        size_t pos;

        InBuf();

        size_t hintImpl() const override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) override;

    public:
        ~InBuf() override;

        InBuf(Input& in);
        InBuf(Input& in, size_t chunkSize);

        InBuf(InBuf&& buf)
            : InBuf()
        {
            buf.xchg(*this);
        }

        InBuf(const InBuf&) = delete;

        void xchg(InBuf& buf);

        Input& stream() {
            return *in_;
        }
    };
}
