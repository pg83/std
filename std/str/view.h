#pragma once

#include "ops.h"

#include <std/sys/types.h>

namespace Std {
    class StringView: public StringOps<StringView> {
        const u8* ptr_;
        size_t len_;

    public:
        inline StringView() noexcept
            : StringView(nullptr, 0)
        {
        }

        template <typename Other>
        inline StringView(const Other& str) noexcept
            : StringView(str.data(), str.length())
        {
        }

        template <size_t N>
        inline StringView(const u8 (&str)[N])
            : StringView(str, N - 1)
        {
        }

        inline StringView(const u8* ptr, size_t len) noexcept
             : ptr_(ptr)
             , len_(len)
        {
        }

        inline auto data() noexcept {
            return ptr_;
        }

        inline auto data() const noexcept {
            return ptr_;
        }

        inline auto length() const noexcept {
            return len_;
        }
    };
}
