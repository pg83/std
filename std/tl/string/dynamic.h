#pragma once

#include "ops.h"

#include <std/os/types.h>

#include <std/tl/buffer.h>
#include <std/tl/support.h>

namespace Std {
    class DynString: public StringOps<DynString> {
        Buffer buf_;

    public:
        inline DynString() noexcept {
        }

        template <typename Other>
        inline DynString(const Other& str)
            : DynString(str.data(), str.size())
        {
        }

        inline DynString(DynString&& str) noexcept
            : buf_(move(str.buf_))
        {
        }

        inline DynString(const u8* ptr, size_t len)
            : buf_(ptr, len)
        {
        }

        inline auto data() noexcept {
            return (c8*)buf_.data();
        }

        inline auto data() const noexcept {
            return (const c8*)buf_.data();
        }

        inline size_t length() const noexcept {
            return buf_.used();
        }

        inline size_t capacity() const noexcept {
            return buf_.capacity();
        }

        inline void append(const c8* ptr, size_t len) {
            buf_.append(ptr, len);
        }

        inline void pushBack(c8 ch) {
            append(&ch, 1);
        }

        inline void grow(size_t len) {
            buf_.grow(len);
        }

        const char* cStr();
    };
}
