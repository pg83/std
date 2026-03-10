#pragma once

#include <std/sys/types.h>

namespace stl {
    class RingBuffer {
        void** buf_;
        size_t capacity_;
        size_t head_;
        size_t tail_;
        size_t size_;

    public:
        RingBuffer(void** buf, size_t capacity) noexcept
            : buf_(buf)
            , capacity_(capacity)
            , head_(0)
            , tail_(0)
            , size_(0)
        {
        }

        bool empty() const noexcept {
            return size_ == 0;
        }

        bool full() const noexcept {
            return size_ == capacity_;
        }

        size_t size() const noexcept {
            return size_;
        }

        void push(void* v) noexcept {
            buf_[tail_] = v;
            tail_ = (tail_ + 1) % capacity_;
            ++size_;
        }

        void* pop() noexcept {
            void* v = buf_[head_];
            head_ = (head_ + 1) % capacity_;
            --size_;
            return v;
        }
    };
}
