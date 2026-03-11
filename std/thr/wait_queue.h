#pragma once

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class WaitQueue: public ARC {
    public:
        using Ref = IntrusivePtr<WaitQueue>;

        struct Item {
            Item* next = nullptr;
            u8 index = 0;
        };

        virtual ~WaitQueue() noexcept;

        virtual void enqueue(Item* item) noexcept = 0;
        virtual Item* dequeue() noexcept = 0;
        virtual size_t sleeping() const noexcept = 0;

        static Ref construct(size_t maxWaiters);
    };
}
