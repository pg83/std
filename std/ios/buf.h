#pragma once

#include "param.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/lib/support.h>

namespace Std {
    class OutBuf;
    struct Output;

    template <typename T>
    void output(OutBuf& out, Meta::FuncParam<T> t);

    class OutBuf {
        Output* out_;
        Buffer buf_;

        OutBuf() noexcept;

    public:
        ~OutBuf();

        OutBuf(Output& out) noexcept;

        inline OutBuf(OutBuf&& buf) noexcept
            : OutBuf()
        {
            buf.swap(*this);
        }

        void swap(OutBuf& buf) noexcept;

        void write(const void* ptr, size_t len);

        // zero-copy
        size_t imbue(void** ptr) noexcept;
        void bump(size_t len) noexcept;

        // non-recursive
        void flush();
        void finish();

        inline Output& stream() noexcept {
            return *out_;
        }

        template <typename Out, typename T>
        friend inline Out& operator<<(Out& out, const T& t) {
            output<T>(out, t);

            return out;
        }

        template <typename Out, typename T>
        friend inline Out&& operator<<(Out&& out, const T& t) {
            output<T>(out, t);

            return move(out);
        }
    };
}
