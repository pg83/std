#pragma once

namespace stl {
    struct CoroExecutor;

    class Barrier {
        struct Impl;
        Impl* impl;

    public:
        explicit Barrier(int n);
        Barrier(int n, CoroExecutor* exec);

        ~Barrier() noexcept;

        void wait() noexcept;
    };
}
