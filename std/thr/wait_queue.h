#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct WaitQueue {
        struct Item {
            Item* next = nullptr;
            u8 index = 0;
        };

        virtual ~WaitQueue() noexcept;

        virtual Item* dequeue() noexcept = 0;
        virtual size_t sleeping() const noexcept = 0;
        virtual void enqueue(Item* item) noexcept = 0;

        static WaitQueue* construct(ObjPool* pool, size_t maxWaiters);
    };
}
