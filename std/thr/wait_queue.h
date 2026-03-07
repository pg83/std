#pragma once

#include <std/sys/types.h>

namespace stl {
    class WaitQueue {
        struct Impl;
        Impl* impl;

    public:
        struct Item {
            Item* next = nullptr;

            virtual u8 index() const noexcept = 0;
        };

        WaitQueue();
        ~WaitQueue() noexcept;

        void enqueue(Item* item) noexcept;
        Item* dequeue() noexcept;
    };
}
