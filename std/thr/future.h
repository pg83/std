#pragma once

#include "semaphore.h"

namespace stl {
    struct CoroExecutor;

    class Future {
        Semaphore sem_;
        void* value_;

    public:
        Future() noexcept;
        explicit Future(CoroExecutor* exec) noexcept;

        ~Future() noexcept;

        void* wait() noexcept;
        void post(void* value) noexcept;

        // can be called only after wait
        void* posted() const noexcept {
            return value_;
        }

        // can be called only after wait
        void* release() noexcept;
    };
}
