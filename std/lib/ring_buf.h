#pragma once

#include <std/sys/types.h>

namespace stl {
    class RingBuffer {
        void** buf_;

        size_t capa_;
        size_t head_;
        size_t tail_;
        size_t size_;

    public:
        RingBuffer(void** buf, size_t capacity) noexcept;
        RingBuffer(void** buf, size_t capacity, size_t prefilled) noexcept;

        bool empty() const noexcept {
            return size_ == 0;
        }

        bool full() const noexcept {
            return size_ == capa_;
        }

        size_t size() const noexcept {
            return size_;
        }

        size_t capacity() const noexcept {
            return capa_;
        }

        void push(void* v) noexcept;
        void* pop() noexcept;

        void linearizeTo(void** dst) const noexcept;
    };
}
