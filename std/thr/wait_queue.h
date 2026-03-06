#pragma once

namespace Std {
    class WaitQueue {
        struct Impl;
        Impl* impl;

    public:
        struct Item {
            Item* next = nullptr;

            virtual void notify() noexcept = 0;
        };

        WaitQueue();
        ~WaitQueue() noexcept;

        void enqueue(Item* item) noexcept;
        Item* dequeue() noexcept;
        bool notifyOne() noexcept;
    };
}
