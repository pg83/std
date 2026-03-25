#include "semaphore.h"
#include "coro.h"
#include "semaphore_iface.h"

#include <std/dbg/insist.h>

#if defined(__APPLE__)
    #include <dispatch/dispatch.h>
#else
    #include <semaphore.h>
#endif

using namespace stl;

namespace {
#if defined(__APPLE__)
    struct SemImpl: public SemaphoreIface {
        dispatch_semaphore_t sem_;

        SemImpl(size_t initial) noexcept
            : sem_(dispatch_semaphore_create(initial))
        {
        }

        ~SemImpl() noexcept override {
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
#else
    struct SemImpl: public SemaphoreIface {
        sem_t sem_;

        SemImpl(size_t initial) noexcept {
            STD_INSIST(sem_init(&sem_, 0, initial) == 0);
        }

        ~SemImpl() noexcept override {
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

Semaphore::Semaphore(size_t initial)
    : impl_(new SemImpl(initial))
{
}

Semaphore::Semaphore(size_t initial, CoroExecutor* exec)
    : impl_(exec->createSemaphore(initial))
{
}

Semaphore::~Semaphore() noexcept {
    delete impl_;
}

void Semaphore::post() noexcept {
    impl_->post();
}

void Semaphore::wait() noexcept {
    impl_->wait();
}

bool Semaphore::tryWait() noexcept {
    return impl_->tryWait();
}
