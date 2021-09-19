#pragma once

#include "ize.h"
#include "ops.h"
#include "view.h"

#include <std/sys/types.h>
#include <std/lib/vector.h>
#include <std/typ/support.h>

namespace Std {
    using DynStringBase = Vector<u8>;

    struct DynString: public DynStringBase {
        inline DynString() noexcept = default;

        template <class Other>
        inline DynString(const Other& str) {
            *this += str;
        }

        inline DynString(const DynString&) = default;

        inline DynString(DynString&& str) noexcept {
            str.xchg(*this);
        }

        char* cStr();

        inline DynString& append(const u8* ptr, size_t len) {
            DynStringBase::append(ptr, ptr + len);

            return *this;
        }

        template <typename Other>
        friend inline DynString operator+(const DynString& l, const Other& r) {
            return move(DynString(l) += r);
        }

        template <typename Other>
        inline DynString& operator+=(const Other& r) {
            auto tmp = stringize(r);

            return append(tmp.data(), tmp.length());
        }
    };
}
