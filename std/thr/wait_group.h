#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class WaitGroup {
        struct Impl;
        Impl* impl;

    public:
        WaitGroup();
        WaitGroup(CoroExecutor* exec);

        WaitGroup(size_t init);
        WaitGroup(size_t init, CoroExecutor* exec);

        ~WaitGroup() noexcept;

        void done() noexcept;
        void wait() noexcept;

        void inc() noexcept {
            add(1);
        }

        void add(size_t n) noexcept;
    };
}
