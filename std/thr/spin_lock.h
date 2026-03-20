#pragma once

namespace stl {
    class CoroExecutor;

    struct SpinLock {
        CoroExecutor* exec_ = nullptr;
        char flag_ = 0;

        void lock() noexcept;
        void unlock() noexcept;
    };
}
