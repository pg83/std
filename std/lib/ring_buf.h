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
        RingBuffer(void** buf, size_t capacity) noexcept
            : buf_(buf)
            , capa_(capacity)
            , head_(0)
            , tail_(0)
            , size_(0)
        {
        }

        bool empty() const noexcept {
            return size_ == 0;
        }

        bool full() const noexcept {
            return size_ == capa_;
        }

        size_t size() const noexcept {
            return size_;
        }

        void push(void* v) noexcept;
        void* pop() noexcept;
    };
}
