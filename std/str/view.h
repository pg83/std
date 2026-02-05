#pragma once

#include <std/sys/types.h>

namespace Std {
    class Buffer;

    class StringView {
        const u8* ptr_;
        size_t len_;

    public:
        inline StringView() noexcept
            : StringView(nullptr, (size_t)0)
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

        inline StringView(const u8* b, const u8* e) noexcept
            : StringView(b, e - b)
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

        // string ops
        StringView prefix(size_t len) const noexcept;
        StringView suffix(size_t len) const noexcept;

        bool startsWith(StringView prefix) const noexcept;
        bool endsWith(StringView suffix) const noexcept;

        const u8* search(StringView substr) const noexcept;

        // hash ops
        u32 hash32() const noexcept;
        u64 hash64() const noexcept;
    };

    bool operator==(const StringView& l, const StringView& r) noexcept;
    bool operator!=(const StringView& l, const StringView& r) noexcept;
    bool operator<(const StringView& l, const StringView& r) noexcept;
}
