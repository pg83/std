#pragma once

namespace stl {
    struct SemaphoreIface;
    struct CoroExecutor;

    class Semaphore {
        SemaphoreIface* impl_;

    public:
        explicit Semaphore(int initial = 0);
        Semaphore(int initial, CoroExecutor* exec);

        ~Semaphore() noexcept;

        void post() noexcept;
        void wait() noexcept;
    };
}
