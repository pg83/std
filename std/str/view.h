#pragma once

#include "hash.h"

#include <std/sys/types.h>

namespace Std {
    class Buffer;

    class StringView {
        const u8* ptr_;
        size_t len_;

    public:
        inline StringView() noexcept
            : StringView(nullptr, 0)
        {
        }

        template <size_t N>
        inline StringView(const u8 (&str)[N]) noexcept
            : StringView(str, N - 1)
        {
        }

        inline StringView(const u8* ptr, size_t len) noexcept
            : ptr_(ptr)
            , len_(len)
        {
        }

        StringView(const char* s) noexcept;
        StringView(const Buffer& b) noexcept;

        inline auto data() const noexcept {
            return ptr_;
        }

        inline auto length() const noexcept {
            return len_;
        }

        // iterator ops
        inline auto begin() const noexcept {
            return data();
        }

        inline auto end() const noexcept {
            return begin() + length();
        }

        inline const auto& operator[](size_t i) const noexcept {
            return *(begin() + i);
        }

        inline bool empty() const noexcept {
            return length() == 0;
        }

        inline const auto& back() const noexcept {
            return *(end() - 1);
        }

        u32 hash32() const noexcept;
        u64 hash64() const noexcept;
    };

    int spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept;

    inline int spaceship(const StringView& l, const StringView& r) noexcept {
        return spaceship(l.data(), l.length(), r.data(), r.length());
    }

    inline bool operator==(const StringView& l, const StringView& r) noexcept {
        return spaceship(l, r) == 0;
    }

    inline bool operator!=(const StringView& l, const StringView& r) noexcept {
        return !(l == r);
    }

    inline bool operator<(const StringView& l, const StringView& r) noexcept {
        return spaceship(l, r) < 0;
    }
}
