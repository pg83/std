#pragma once

#include "buffer.h"

#include <std/typ/intrin.h>

namespace Std {
    template <typename T>
    class Vector {
        Buffer buf_;

    public:
        static_assert(stdHasTrivialDestructor(T));

        inline Vector() = default;
        inline Vector(Vector&&) = default;
        inline Vector(const Vector&) = default;

        inline auto data() const noexcept {
            return (const T*)buf_.data();
        }

        inline auto begin() const noexcept {
            return data();
        }

        inline auto end() const noexcept {
            return (const T*)buf_.current();
        }

        inline auto storageEnd() const noexcept {
            return (const T*)buf_.storageEnd();
        }

        inline auto mutData() noexcept {
            return const_cast<T*>(data());
        }

        inline auto mutBegin() noexcept {
            return const_cast<T*>(begin());
        }

        inline auto mutEnd() noexcept {
            return const_cast<T*>(end());
        }

        inline auto mutStorageEnd() noexcept {
            return const_cast<T*>(storageEnd());
        }

        inline size_t left() const noexcept {
            return storageEnd() - end();
        }

        inline size_t length() const noexcept {
            return end() - begin();
        }

        inline bool empty() const noexcept {
            return buf_.empty();
        }

        inline auto& mut(size_t i) noexcept {
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

        inline void clear() noexcept {
            buf_.reset();
        }

        inline void grow(size_t len) {
            buf_.grow(len * sizeof(T));
        }

        inline void growDelta(size_t delta) {
            grow(length() + delta);
        }

        inline void pushBack(const T& t) {
            buf_.append(&t, sizeof(t));
        }

        inline auto popBack() {
            auto res = back();

            buf_.seekAbsolute(end() - 1);

            return res;
        }

        inline void append(const T* b, const T* e) {
            buf_.append((const u8*)b, (const u8*)e - (const u8*)b);
        }

        inline void append(const T* b, size_t len) {
            append(b, b + len);
        }

        inline void xchg(Vector& v) noexcept {
            buf_.xchg(v.buf_);
        }
    };
}
