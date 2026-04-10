#pragma once

#include "task.h"

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    class IntrusiveList;

    struct CondVarIface;

    struct ThreadPoolHooks {
        virtual CondVarIface* createCondVar(size_t index) = 0;
    };

    struct ThreadPool {
        virtual void join() noexcept = 0;
        virtual bool workerId(size_t* id) noexcept = 0;
        virtual void flushLocal() noexcept = 0;
        virtual void submitTasks(IntrusiveList& tasks) noexcept = 0;

        void submitTask(Task* task) noexcept;

        template <typename F>
        void submit(F f) {
            submitTask(makeTask(f));
        }

        static ThreadPool* sync(ObjPool* pool);
        static ThreadPool* simple(ObjPool* pool, size_t threads);
        static ThreadPool* workStealing(ObjPool* pool, size_t threads);
        static ThreadPool* workStealing(ObjPool* pool, size_t threads, ThreadPoolHooks* hooks);
    };
}
