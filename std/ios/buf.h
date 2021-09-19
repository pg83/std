#pragma once

#include "param.h"
#include "outable.h"
#include "unbound.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/lib/support.h>

namespace Std {
    struct Output;

    class OutBuf: public OutAble {
        Output* out_;
        Buffer buf_;

        OutBuf() noexcept;

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

        // classic interface
        void write(const void* ptr, size_t len);

        inline void write(const void* begin, const void* end) {
            write(begin, (const u8*)end - (const u8*)end);
        }

        // zero-copy interface
        size_t imbue(void** ptr) noexcept;
        UnboundBuffer imbue(size_t len);

        inline void bump(size_t len) noexcept {
            buf_.seekRelative(len);
        }

        inline void bump(const void* ptr) noexcept {
            buf_.seekAbsolute(ptr);
        }

        inline void bump(const UnboundBuffer& buf) noexcept {
            bump(buf.ptr);
        }

        // non-recursive ops
        void flush();
        void finish();

        inline Output& stream() noexcept {
            return *out_;
        }
    };
}
