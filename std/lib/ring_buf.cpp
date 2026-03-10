#include "ring_buf.h"

using namespace stl;

void RingBuffer::push(void* v) noexcept {
    buf_[tail_] = v;
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
}

void* RingBuffer::pop() noexcept {
    void* v = buf_[head_];

    head_ = (head_ + 1) % capacity_;
    --size_;

    return v;
}
