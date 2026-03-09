#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class WaitGroup {
        struct Impl;
        Impl* impl;

    public:
        WaitGroup();
        explicit WaitGroup(CoroExecutor* exec);

        ~WaitGroup() noexcept;

        void add(size_t n) noexcept;
        void done() noexcept;
        void wait() noexcept;
    };
}
