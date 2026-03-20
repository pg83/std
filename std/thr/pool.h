#pragma once

#include "task.h"

#include <std/sys/types.h>

namespace stl {
    class PCG32;
    class ObjPool;

    struct ThreadPool {
        virtual ~ThreadPool() noexcept;

        virtual void join() noexcept = 0;
        virtual PCG32& random() noexcept = 0;
        virtual void** tls(u64 key) noexcept = 0;
        virtual void submitTask(Task* task) noexcept = 0;

        virtual void steal() noexcept;

        template <typename F>
        void submit(F f) {
            submitTask(makeTask(f));
        }

        static ThreadPool* sync(ObjPool* pool);
        static ThreadPool* simple(ObjPool* pool, size_t threads);
        static ThreadPool* workStealing(ObjPool* pool, size_t threads);

        static u64 registerTlsKey() noexcept;
    };
}
