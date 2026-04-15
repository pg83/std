#include "semaphore.h"
#include "coro.h"

#include <std/dbg/insist.h>
#include <std/mem/obj_pool.h>

#if defined(__APPLE__)
    #include <dispatch/dispatch.h>
#endif

#if defined(__linux__)
    #include <semaphore.h>
#endif

using namespace stl;

void* Semaphore::nativeHandle() noexcept {
    return nullptr;
}

namespace {
#if defined(__APPLE__)
    struct SemImpl: public Semaphore {
        dispatch_semaphore_t sem_;

        SemImpl(size_t initial) noexcept
            : sem_(dispatch_semaphore_create((long)initial))
        {
        }

        ~SemImpl() noexcept {
            dispatch_release(sem_);
        }

        void post() noexcept override {
            dispatch_semaphore_signal(sem_);
        }

        void wait() noexcept override {
            dispatch_semaphore_wait(sem_, DISPATCH_TIME_FOREVER);
        }

        bool tryWait() noexcept override {
            return dispatch_semaphore_wait(sem_, DISPATCH_TIME_NOW) == 0;
        }
    };
#elif defined(__linux__)
    struct SemImpl: public Semaphore {
        sem_t sem_;

        SemImpl(size_t initial) noexcept {
            STD_INSIST(sem_init(&sem_, 0, initial) == 0);
        }

        ~SemImpl() noexcept {
            STD_INSIST(sem_destroy(&sem_) == 0);
        }

        void post() noexcept override {
            STD_INSIST(sem_post(&sem_) == 0);
        }

        void wait() noexcept override {
            STD_INSIST(sem_wait(&sem_) == 0);
        }

        bool tryWait() noexcept override {
            return sem_trywait(&sem_) == 0;
        }
    };
#endif
}

Semaphore* Semaphore::create(ObjPool* pool, size_t initial) {
    return pool->make<SemImpl>(initial);
}

Semaphore* Semaphore::create(ObjPool* pool, size_t initial, CoroExecutor* exec) {
    return exec->createSemaphore(pool, initial);
}
