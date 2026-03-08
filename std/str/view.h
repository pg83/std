#pragma once

#include <std/sys/types.h>

namespace stl {
    class Buffer;

    class StringView {
        const u8* ptr_;
        size_t len_;

    public:
        StringView()
            : StringView(nullptr, (size_t)0)
        {
        }

        template <size_t N>
        StringView(const u8 (&str)[N])
            : StringView(str, N - 1)
        {
        }

        StringView(const u8* ptr, size_t len)
            : ptr_(ptr)
            , len_(len)
        {
        }

        StringView(const u8* b, const u8* e)
            : StringView(b, e - b)
        {
        }

        StringView(const char* s);
        StringView(const Buffer& b);

        auto data() const {
            return ptr_;
        }

        auto length() const {
            return len_;
        }

        // iterator ops
        auto begin() const {
            return data();
        }

        auto end() const {
            return begin() + length();
        }

        const auto& operator[](size_t i) const {
            return *(begin() + i);
        }

        bool empty() const {
            return length() == 0;
        }

        const auto& back() const {
            return *(end() - 1);
        }

        // string ops
        StringView prefix(size_t len) const;
        StringView suffix(size_t len) const;

        bool endsWith(StringView suffix) const;
        bool startsWith(StringView prefix) const;

        const u8* memChr(u8 ch) const;
        const u8* search(StringView substr) const;

        // hash ops
        u32 hash32() const;
        u64 hash64() const;

        // parse
        u64 stou() const;
    };

    bool operator==(StringView l, StringView r);
    bool operator!=(StringView l, StringView r);
    bool operator<(StringView l, StringView r);
}
