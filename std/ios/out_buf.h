#pragma once

#include "out_zc.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace stl {
    class OutBuf: public ZeroCopyOutput {
        Output* out_;
        Buffer buf_;
        size_t chunk;

        OutBuf();

        size_t writeDirect(const void* ptr, size_t len);
        size_t writeMultipart(const void* ptr, size_t len);

        // state
        void flushImpl() override;
        void finishImpl() override;

        // classic
        size_t writeImpl(const void* ptr, size_t len) override;
        size_t hintImpl() const override;

        // zero-copy
        void* imbueImpl(size_t* len) override;
        void commitImpl(size_t len) override;

    public:
        ~OutBuf() override;

        OutBuf(Output& out);
        OutBuf(Output& out, size_t chunkSize);

        OutBuf(OutBuf&& buf)
            : OutBuf()
        {
            buf.xchg(*this);
        }

        OutBuf(const OutBuf&) = delete;

        void xchg(OutBuf& buf);

        Output& stream() {
            return *out_;
        }
    };
}
