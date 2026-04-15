#include "mutex.h"
#include "coro.h"

#include <std/str/view.h>
#include <std/mem/obj_pool.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>
#include <sched.h>

using namespace stl;

namespace {
    struct alignas(64) SpinMutexImpl: public Mutex {
        CoroExecutor* exec_;
        char flag_ = 0;

        SpinMutexImpl(CoroExecutor* exec) noexcept
            : exec_(exec)
        {
        }

        void spin() noexcept {
            if (exec_ && exec_->currentCoroId()) {
                exec_->yield();
            } else {
                sched_yield();
            }
        }

        void* nativeHandle() noexcept override {
            return exec_;
        }

        void wait() noexcept override {
            for (;;) {
                if (!__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE)) {
                    return;
                }

                while (__atomic_load_n(&flag_, __ATOMIC_RELAXED)) {
                    spin();
                }
            }
        }

        void post() noexcept override {
            __atomic_clear(&flag_, __ATOMIC_RELEASE);
        }

        bool tryWait() noexcept override {
            return !__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE);
        }
    };

    struct PosixMutexImpl: public Mutex, public pthread_mutex_t {
        PosixMutexImpl() {
            if (pthread_mutex_init(this, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_mutex_init failed"));
            }
        }

        ~PosixMutexImpl() noexcept {
            STD_INSIST(pthread_mutex_destroy(this) == 0);
        }

        void wait() noexcept override {
            STD_INSIST(pthread_mutex_lock(this) == 0);
        }

        void post() noexcept override {
            STD_INSIST(pthread_mutex_unlock(this) == 0);
        }

        bool tryWait() noexcept override {
            return pthread_mutex_trylock(this) == 0;
        }

        void* nativeHandle() noexcept override {
            return static_cast<pthread_mutex_t*>(this);
        }
    };
}

Mutex* Mutex::create(ObjPool* pool) {
    return pool->make<PosixMutexImpl>();
}

Mutex* Mutex::create(ObjPool* pool, CoroExecutor* exec) {
    return exec->createMutex(pool);
}

Mutex* Mutex::createSpinLock(ObjPool* pool) {
    return pool->make<SpinMutexImpl>(nullptr);
}

Mutex* Mutex::createSpinLock(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<SpinMutexImpl>(exec);
}
