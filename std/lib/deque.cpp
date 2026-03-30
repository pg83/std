#include "deque.h"
#include "ring_buf.h"

#include <std/sys/crt.h>
#include <std/alg/bits.h>
#include <std/alg/exchange.h>

using namespace stl;

struct Deque::Impl: public RingBuffer {
    Impl(size_t capacity) noexcept
        : RingBuffer((void**)(this + 1), capacity)
    {
    }

    Impl(size_t capacity, size_t prefilled) noexcept
        : RingBuffer((void**)(this + 1), capacity, prefilled)
    {
    }

    void* operator new(size_t, void* p) noexcept {
        return p;
    }

    void operator delete(void* p) noexcept {
        freeMemory(p);
    }

    static void* allocRaw(size_t minCap, size_t* realCap) {
        auto alloc = clp2(sizeof(Impl) + minCap * sizeof(void*));
        auto mem = allocateMemory(alloc);

        *realCap = (alloc - sizeof(Impl)) / sizeof(void*);

        return mem;
    }

    static Impl* create(size_t minCap) {
        size_t cap;
        auto mem = allocRaw(minCap, &cap);

        return new (mem) Impl(cap);
    }

    Impl* regrow() {
        size_t cap;
        auto mem = allocRaw(capacity() * 2, &cap);

        linearizeTo((void**)((u8*)mem + sizeof(Impl)));

        return new (mem) Impl(cap, size());
    }

    void pushBack(void* v) {
        push(v);
    }

    void* popFront() noexcept {
        return pop();
    }
};

Deque::Deque()
    : Deque(8)
{
}

Deque::Deque(size_t capacity)
    : impl_(Impl::create(capacity))
{
}

Deque::~Deque() noexcept {
    delete impl_;
}

bool Deque::empty() const noexcept {
    return impl_->empty();
}

bool Deque::full() const noexcept {
    return impl_->full();
}

size_t Deque::size() const noexcept {
    return impl_->size();
}

void Deque::pushBack(void* v) {
    if (impl_->full()) {
        delete exchange(impl_, impl_->regrow());
    }

    impl_->pushBack(v);
}

void* Deque::popFront() noexcept {
    return impl_->popFront();
}
