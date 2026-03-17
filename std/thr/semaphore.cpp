#include "semaphore.h"
#include "semaphore_iface.h"
#include "coro.h"

#include <semaphore.h>

#include <std/dbg/insist.h>

using namespace stl;

namespace {
    struct SemImpl: public SemaphoreIface {
        sem_t sem_;

        SemImpl(int initial) noexcept {
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
    };
}

Semaphore::Semaphore(int initial)
    : impl_(new SemImpl(initial))
{
}

Semaphore::Semaphore(int initial, CoroExecutor* exec)
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
