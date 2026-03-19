#include "mutex.h"
#include "coro.h"
#include "semaphore_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

namespace {
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
    delete impl;
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
