#pragma once

#include "buffer.h"

namespace Std {
    template <typename T>
    class Vector {
        Buffer buf_;

    public:
        inline Vector() = default;
        inline Vector(Vector&&) = default;
        inline Vector(const Vector&) = default;

        inline auto begin() const noexcept {
            return (const T*)buf_.data();
        }

        inline auto end() const noexcept {
            return (const T*)((const char*)buf_.data() + buf_.length());
        }

        inline auto mutBegin() noexcept {
            return (T*)buf_.data();
        }

        inline auto mutEnd() noexcept {
            return (T*)((char*)buf_.data() + buf_.length());
        }

        inline size_t length() const noexcept {
            return end() - begin();
        }

        inline void pushBack(const T& t) {
            buf_.append(&t, sizeof(t));
        }

        inline bool empty() const noexcept {
            return buf_.empty();
        }

        inline auto& operator[](size_t i) noexcept {
            return *(mutBegin() + i);
        }

        inline const auto& operator[](size_t i) const noexcept {
            return *(begin() + i);
        }

        inline const auto& back() const noexcept {
            return *(end() - 1);
        }

        inline auto& mutBack() noexcept {
            return *(mutEnd() - 1);
        }
    };
}
