#include "semaphore.h"
#include "coro.h"
#include "semaphore_iface.h"

#include <std/dbg/insist.h>

#include <semaphore.h>

using namespace stl;

namespace {
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
