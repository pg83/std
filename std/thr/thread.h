#pragma once

#include "runable.h"

#include <std/sys/types.h>
#include <std/mem/obj_pool.h>

namespace stl {
    struct CoroExecutor;

    struct Thread {
        virtual void start(Runable& runable) = 0;
        virtual void join() noexcept = 0;
        virtual u64 threadId() const noexcept = 0;

        static u64 currentThreadId() noexcept;

        static Thread* create(ObjPool* pool, Runable& runable);
        static Thread* create(ObjPool* pool, CoroExecutor* exec, Runable& runable);
    };

    class ScopedThread {
        ObjPool::Ref pool_;
        Thread* thr_;

    public:
        template <typename T>
        ScopedThread(T functor)
            : pool_(ObjPool::fromMemory())
            , thr_(Thread::create(pool_.mutPtr(), *makeRunablePtr(functor)))
        {
        }

        ~ScopedThread() noexcept {
            thr_->join();
        }
    };
}
