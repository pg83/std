#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    struct Channel {
        struct Impl;
        Impl* impl_;

        Channel();
        Channel(size_t cap);
        explicit Channel(CoroExecutor* exec);
        Channel(CoroExecutor* exec, size_t cap);

        ~Channel() noexcept;

        void enqueue(void* v) noexcept;
        bool dequeue(void** out) noexcept;

        bool tryEnqueue(void* v) noexcept;
        bool tryDequeue(void** out) noexcept;

        void close() noexcept;
    };
}
