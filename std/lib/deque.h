#pragma once

#include <std/sys/types.h>

namespace stl {
    class Deque {
        struct Impl;
        Impl* impl_;

    public:
        Deque();
        explicit Deque(size_t capacity);

        ~Deque() noexcept;

        bool empty() const noexcept;
        bool full() const noexcept;
        size_t size() const noexcept;

        void pushBack(void* v);
        void* popFront() noexcept;
    };
}
