#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct WaitGroup {
        virtual void done() noexcept = 0;
        virtual void wait() noexcept = 0;
        virtual void add(size_t n) noexcept = 0;

        void inc() noexcept {
            add(1);
        }

        static WaitGroup* create(ObjPool* pool, size_t init);
        static WaitGroup* create(ObjPool* pool, size_t init, CoroExecutor* exec);
    };
}
