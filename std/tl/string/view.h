#pragma once

#include "ops.h"

#include <std/os/types.h>

namespace Std {
    class StringView: public StringOps<StringView> {
        const c8* ptr_;
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

        inline StringView(const c8* ptr, size_t len) noexcept
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
