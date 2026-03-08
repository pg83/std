#pragma once

#include "buffer.h"

#include <std/typ/intrin.h>

namespace stl {
    template <typename T>
    class Vector {
        Buffer buf_;

    public:
        static_assert(stdHasTrivialDestructor(T));

        Vector(size_t reserve) {
            grow(reserve);
        }

        Vector() = default;
        Vector(Vector&&) = default;
        Vector(const Vector&) = default;

        auto data() const {
            return (const T*)buf_.data();
        }

        auto begin() const {
            return data();
        }

        auto end() const {
            return (const T*)buf_.current();
        }

        auto storageEnd() const {
            return begin() + capacity();
        }

        auto mutData() {
            return const_cast<T*>(data());
        }

        auto mutBegin() {
            return const_cast<T*>(begin());
        }

        auto mutEnd() {
            return const_cast<T*>(end());
        }

        auto mutStorageEnd() {
            return const_cast<T*>(storageEnd());
        }

        size_t left() const {
            return capacity() - length();
        }

        size_t capacity() const {
            return buf_.capacity() / sizeof(T);
        }

        size_t length() const {
            return end() - begin();
        }

        bool empty() const {
            return buf_.empty();
        }

        auto& mut(size_t i) {
            return *(mutBegin() + i);
        }

        const auto& operator[](size_t i) const {
            return *(begin() + i);
        }

        const auto& back() const {
            return *(end() - 1);
        }

        auto& mutBack() {
            return *(mutEnd() - 1);
        }

        void clear() {
            buf_.reset();
        }

        void grow(size_t len) {
            buf_.grow(len * sizeof(T));
        }

        void growDelta(size_t delta) {
            grow(length() + delta);
        }

        void pushBack(const T& t) {
            buf_.append(&t, sizeof(t));
        }

        auto popBack() {
            auto res = back();

            buf_.seekAbsolute(end() - 1);

            return res;
        }

        void append(const T* b, const T* e) {
            buf_.append((const u8*)b, (const u8*)e - (const u8*)b);
        }

        void append(const T* b, size_t len) {
            append(b, b + len);
        }

        void xchg(Vector& v) {
            buf_.xchg(v.buf_);
        }

        void zero(size_t cnt) {
            buf_.zero(cnt * sizeof(T));
        }
    };
}
