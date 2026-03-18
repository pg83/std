#pragma once

#include "semaphore.h"

namespace stl {
    struct CoroExecutor;

    class Future {
        Semaphore sem_;
        void* value_;

    public:
        Future() noexcept
            : sem_(0)
            , value_(nullptr)
        {
        }

        explicit Future(CoroExecutor* exec) noexcept
            : sem_(0, exec)
            , value_(nullptr)
        {
        }

        void post(void* value) noexcept {
            value_ = value;
            sem_.post();
        }

        void* wait() noexcept {
            sem_.wait();
            return value_;
        }

        void* posted() const noexcept {
            return value_;
        }
    };
}
