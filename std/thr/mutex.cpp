#include "mutex.h"
#include "coro.h"
#include "semaphore_iface.h"

#include <std/str/view.h>
#include <std/mem/obj_pool.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>
#include <sched.h>

using namespace stl;

namespace {
    struct alignas(64) SpinSemImpl: public SemaphoreIface {
        CoroExecutor* exec_;
        char flag_ = 0;

        SpinSemImpl(CoroExecutor* exec) noexcept
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

    struct PosixMutexImpl: public SemaphoreIface, public pthread_mutex_t {
        PosixMutexImpl() {
            if (pthread_mutex_init(this, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_mutex_init failed"));
            }
        }

        ~PosixMutexImpl() noexcept override {
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

Mutex::Mutex()
    : Mutex(new PosixMutexImpl())
{
}

Mutex::Mutex(CoroExecutor* exec)
    : Mutex(exec->createSemaphore(1))
{
}

SemaphoreIface* Mutex::defaultImpl(ObjPool* pool) {
    struct Impl: public PosixMutexImpl {
        bool owned() const noexcept override {
            return true;
        }
    };

    return pool->make<Impl>();
}

Mutex* Mutex::createDefault(ObjPool* pool) {
    return pool->make<Mutex>(defaultImpl(pool));
}

Mutex* Mutex::createSpinLock(ObjPool* pool) {
    return createSpinLock(pool, nullptr);
}

Mutex* Mutex::createSpinLock(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<Mutex>(spinLock(pool, exec));
}

SemaphoreIface* Mutex::spinLock(CoroExecutor* exec) {
    return new SpinSemImpl(exec);
}

SemaphoreIface* Mutex::spinLock(ObjPool* pool, CoroExecutor* exec) {
    struct Impl: public SpinSemImpl {
        using SpinSemImpl::SpinSemImpl;

        bool owned() const noexcept override {
            return true;
        }
    };

    return pool->make<Impl>(exec);
}

Mutex::Mutex(SemaphoreIface* iface)
    : impl(iface)
{
}

Mutex::Mutex(bool locked)
    : Mutex()
{
    if (locked) {
        lock();
    }
}

Mutex::~Mutex() noexcept {
    if (!impl->owned()) {
        delete impl;
    }
}

void Mutex::lock() noexcept {
    impl->wait();
}

void Mutex::unlock() noexcept {
    impl->post();
}

bool Mutex::tryLock() noexcept {
    return impl->tryWait();
}

void* Mutex::nativeHandle() noexcept {
    return impl->nativeHandle();
}
