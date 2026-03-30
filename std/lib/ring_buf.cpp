#include "ring_buf.h"

#include <std/sys/crt.h>
#include <std/dbg/assert.h>

using namespace stl;

RingBuffer::RingBuffer(void** buf, size_t capacity) noexcept
    : buf_(buf)
    , capa_(capacity)
    , head_(0)
    , tail_(0)
    , size_(0)
{
}

RingBuffer::RingBuffer(void** buf, size_t capacity, size_t prefilled) noexcept
    : buf_(buf)
    , capa_(capacity)
    , head_(0)
    , tail_(prefilled)
    , size_(prefilled)
{
}

void RingBuffer::push(void* v) noexcept {
    STD_ASSERT(!full());

    buf_[tail_] = v;
    tail_ = (tail_ + 1) % capa_;
    ++size_;
}

void* RingBuffer::pop() noexcept {
    STD_ASSERT(!empty());

    void* v = buf_[head_];

    head_ = (head_ + 1) % capa_;
    --size_;

    return v;
}

void RingBuffer::linearizeTo(void** dst) const noexcept {
    if (!size_) {
        return;
    }

    if (head_ < tail_) {
        memCpy(dst, buf_ + head_, size_ * sizeof(void*));
    } else {
        auto first = capa_ - head_;

        memCpy(dst, buf_ + head_, first * sizeof(void*));
        memCpy(dst + first, buf_, tail_ * sizeof(void*));
    }
}
