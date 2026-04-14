#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    class WaitGroup {
        struct Impl;
        Impl* impl;

    public:
        WaitGroup();
        WaitGroup(Impl* impl);
        WaitGroup(size_t init);
        WaitGroup(CoroExecutor* exec);
        WaitGroup(size_t init, CoroExecutor* exec);

        static WaitGroup* create(ObjPool* pool);
        static WaitGroup* create(ObjPool* pool, size_t init);

        ~WaitGroup() noexcept;

        void done() noexcept;
        void wait() noexcept;

        void inc() noexcept {
            add(1);
        }

        void add(size_t n) noexcept;
    };
}
