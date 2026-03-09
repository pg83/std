#include "barrier.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

#include <std/sys/types.h>

using namespace stl;

struct Barrier::Impl {
    Mutex mutex;
    CondVar cv;
    size_t remaining;

    Impl(size_t n)
        : remaining(n)
    {
    }

    Impl(size_t n, CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
        , remaining(n)
    {
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        if (--remaining == 0) {
            cv.broadcast();
        } else {
            while (remaining > 0) {
                cv.wait(mutex);
            }
        }
    }
};

Barrier::Barrier(size_t n)
    : impl(new Impl(n))
{
}

Barrier::Barrier(size_t n, CoroExecutor* exec)
    : impl(new Impl(n, exec))
{
}

Barrier::~Barrier() noexcept {
    delete impl;
}

void Barrier::wait() noexcept {
    impl->wait();
}
