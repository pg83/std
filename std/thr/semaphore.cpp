#include "semaphore.h"
#include "coro.h"
#include "mutex.h"
#include "guard.h"
#include "cond_var.h"
#include "semaphore_iface.h"

#include <std/dbg/insist.h>

#if defined(__APPLE__)
    #include <dispatch/dispatch.h>
#endif

#if defined(__linux__)
    #include <semaphore.h>
#endif

using namespace stl;

namespace {
#if defined(__APPLE__)
    struct SemImpl: public SemaphoreIface {
        dispatch_semaphore_t sem_;

        SemImpl(size_t initial) noexcept
            : sem_(dispatch_semaphore_create((long)initial))
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
#elif defined(__linux__)
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
#else
    struct SemImpl: public SemaphoreIface {
        Mutex mu_;
        CondVar cv_;
        size_t count_;

        SemImpl(size_t initial) noexcept
            : count_(initial)
        {
        }

        void post() noexcept override {
            LockGuard g(mu_);
            ++count_;
            cv_.signal();
        }

        void wait() noexcept override {
            LockGuard g(mu_);

            while (count_ == 0) {
                cv_.wait(mu_);
            }

            --count_;
        }

        bool tryWait() noexcept override {
            LockGuard g(mu_);

            if (count_ > 0) {
                --count_;
                return true;
            }

            return false;
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
