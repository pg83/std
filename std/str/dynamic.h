#pragma once

#include "ize.h"
#include "ops.h"
#include "view.h"

#include <std/sys/types.h>

#include <std/lib/buffer.h>
#include <std/lib/support.h>

namespace Std {
    class DynString: public StringOps<DynString> {
        Buffer buf_;

    public:
        inline DynString() noexcept = default;

        template <class Other>
        inline DynString(const Other& str) {
            *this += str;
        }

        inline DynString(const DynString&) = default;

        inline DynString(DynString&& str) noexcept
            : buf_(move(str.buf_))
        {
        }

        inline auto data() noexcept {
            return (u8*)buf_.data();
        }

        inline auto data() const noexcept {
            return (const u8*)buf_.data();
        }

        inline size_t length() const noexcept {
            return buf_.used();
        }

        inline size_t capacity() const noexcept {
            return buf_.capacity();
        }

        inline void grow(size_t len) {
            buf_.grow(len);
        }

        char* cStr();

        inline DynString& append(const u8* ptr, size_t len) {
            buf_.append(ptr, len);

            return *this;
        }

        template <typename Other>
        friend inline DynString operator+(const DynString& l, const Other& r) {
            return DynString(l) += r;
        }

        template <typename Other>
        inline DynString& operator+=(const Other& r) {
            auto tmp = stringize(r);

            return append(tmp.data(), tmp.length());
        }
    };
}