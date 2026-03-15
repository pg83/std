#pragma once

#include "wait_group.h"

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class Barrier {
        WaitGroup wg;

    public:
        explicit Barrier(size_t n);
        Barrier(size_t n, CoroExecutor* exec);

        ~Barrier() noexcept;

        void wait() noexcept;
    };
}
