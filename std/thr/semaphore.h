#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct Semaphore {
        virtual void post() noexcept = 0;
        virtual void wait() noexcept = 0;
        virtual bool tryWait() noexcept = 0;
        virtual void* nativeHandle() noexcept;

        static Semaphore* create(ObjPool* pool, size_t initial);
        static Semaphore* create(ObjPool* pool, size_t initial, CoroExecutor* exec);
    };
}
