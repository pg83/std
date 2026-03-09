#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class Barrier {
        struct Impl;
        Impl* impl;

    public:
        explicit Barrier(size_t n);
        Barrier(size_t n, CoroExecutor* exec);

        ~Barrier() noexcept;

        void wait() noexcept;
    };
}
