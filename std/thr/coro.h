#pragma once

#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    class IntrusiveList;

    struct Task;
    struct IoReactor;
    struct ThreadPool;
    struct EventIface;
    struct ThreadIface;
    struct Mutex;
    struct CondVar;
    struct Semaphore;
    struct CoroExecutor;

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;

        SpawnParams() noexcept;

        SpawnParams& setStackPtr(void* v) noexcept;
        SpawnParams& setStackSize(size_t v) noexcept;
        SpawnParams& setRunablePtr(Runable* v) noexcept;
        SpawnParams& setStack(void* v, size_t len) noexcept;
        SpawnParams& setStack(ObjPool* v, size_t len) noexcept;

        template <typename F>
        SpawnParams& setRunable(F f) {
            return setRunablePtr(makeRunablePtr(f));
        }
    };

    struct CoroExecutor {
        virtual void join() noexcept = 0;
        virtual void yield() noexcept = 0;
        virtual void spawnRun(SpawnParams params) = 0;
        virtual void parkWith(Runable&&, Task**) noexcept = 0;
        virtual void offloadRun(ThreadPool* pool, Runable&& work) = 0;

        virtual IoReactor* io() noexcept = 0;
        virtual ThreadPool* pool() noexcept = 0;

        virtual void createEvent(void* buf) = 0;
        virtual CondVar* createCondVar(ObjPool* pool) = 0;
        virtual ThreadIface* createThread(ObjPool* pool) = 0;
        virtual Mutex* createSemaphoreImpl(ObjPool* pool, size_t initial) = 0;

        Semaphore* createSemaphore(ObjPool* pool, size_t initial);
        Mutex* createMutex(ObjPool* pool);

        virtual void* currentCoroId() const noexcept = 0;

        template <typename F>
        void spawn(F f) {
            spawnRun(SpawnParams().setRunable(f));
        }

        template <typename F>
        void offload(ThreadPool* pool, F f) {
            offloadRun(pool, makeRunable(f));
        }

        static CoroExecutor* create(ObjPool* pool, size_t threads);
    };
}
