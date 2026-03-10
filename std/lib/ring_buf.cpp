#include "ring_buf.h"

#include <std/dbg/assert.h>

using namespace stl;

void RingBuffer::push(void* v) noexcept {
    STD_ASSERT(!full());

    buf_[tail_] = v;
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
}

void* RingBuffer::pop() noexcept {
    STD_ASSERT(!empty());

    void* v = buf_[head_];

    head_ = (head_ + 1) % capacity_;
    --size_;

    return v;
}
