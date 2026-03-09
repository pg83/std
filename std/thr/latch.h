#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class Latch {
        struct Impl;
        Impl* impl;

    public:
        explicit Latch(size_t n);
        Latch(size_t n, CoroExecutor* exec);

        ~Latch() noexcept;

        void arrive() noexcept;
        void wait() noexcept;
    };
}
