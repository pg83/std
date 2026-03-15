#pragma once

#include "task.h"
#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    u64 registerTlsKey() noexcept;

    class ObjPool;
    class PCG32;

    struct ThreadPool {
        virtual ~ThreadPool() noexcept;

        virtual void join() noexcept = 0;
        virtual void beforeBlock() noexcept;
        virtual PCG32& random() noexcept = 0;
        virtual void** tls(u64 key) noexcept = 0;
        virtual void submitTask(Task* task) noexcept = 0;

        template <typename F>
        void submit(F f) {
            submitTask(makeTask(f));
        }

        static ThreadPool* sync(ObjPool* pool);
        static ThreadPool* simple(ObjPool* pool, size_t threads);
        static ThreadPool* workStealing(ObjPool* pool, size_t threads);
    };
}
