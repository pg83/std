#include "signal.h"
#include "signal_iface.h"
#include "coro.h"

#include <semaphore.h>

using namespace stl;

namespace {
    struct PosixSignalImpl: public SignalIface {
        sem_t sem_;

        PosixSignalImpl() {
            sem_init(&sem_, 0, 0);
        }

        ~PosixSignalImpl() noexcept {
            sem_destroy(&sem_);
        }

        void set() noexcept override {
            sem_post(&sem_);
        }

        void wait() noexcept override {
            sem_wait(&sem_);
        }
    };
}

Signal::Signal()
    : impl_(new PosixSignalImpl())
{
}

Signal::Signal(CoroExecutor* exec)
    : impl_(exec->createSignal())
{
}

Signal::~Signal() noexcept {
    delete impl_;
}

void Signal::set() noexcept {
    impl_->set();
}

void Signal::wait() noexcept {
    impl_->wait();
}
