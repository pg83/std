#pragma once

#include "ize.h"
#include "ops.h"
#include "view.h"

#include <std/os/types.h>

#include <std/tl/buffer.h>
#include <std/tl/support.h>

namespace Std {
    class DynString: public StringOps<DynString> {
        Buffer buf_;

    public:
        inline DynString() noexcept = default;

        template <class Other>
        inline DynString(const Other& str) {
            *this += str;
        }

        inline DynString(DynString&& str) noexcept
            : buf_(move(str.buf_))
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

        inline void grow(size_t len) {
            buf_.grow(len);
        }

        const char* cStr();

        inline DynString& append(const c8* ptr, size_t len) {
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
