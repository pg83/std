#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;
    struct SemaphoreIface;

    class Semaphore {
        SemaphoreIface* impl_;

    public:
        explicit Semaphore(size_t initial);
        Semaphore(size_t initial, CoroExecutor* exec);

        ~Semaphore() noexcept;

        void post() noexcept;
        void wait() noexcept;
    };
}
