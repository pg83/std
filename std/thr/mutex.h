#pragma once

#include "semaphore.h"

namespace stl {
    struct Mutex: public Semaphore {
        void lock() noexcept {
            wait();
        }

        void unlock() noexcept {
            post();
        }

        bool tryLock() noexcept {
            return tryWait();
        }

        static Mutex* create(ObjPool* pool);
        static Mutex* create(ObjPool* pool, CoroExecutor* exec);

        static Mutex* createSpinLock(ObjPool* pool);
        static Mutex* createSpinLock(ObjPool* pool, CoroExecutor* exec);
    };
}
