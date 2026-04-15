#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    struct Runable;
    struct CoroExecutor;

    struct Thread {
        virtual void start(Runable& runable) = 0;
        virtual void join() noexcept = 0;
        virtual u64 threadId() const noexcept = 0;

        static u64 currentThreadId() noexcept;

        static Thread* create(ObjPool* pool, Runable& runable);
        static Thread* create(ObjPool* pool, CoroExecutor* exec, Runable& runable);
    };
}
