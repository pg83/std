#include "latch.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

using namespace stl;

struct Latch::Impl {
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

    void arrive() noexcept {
        LockGuard lock(mutex);

        if (--remaining == 0) {
            cv.broadcast();
        }
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        while (remaining > 0) {
            cv.wait(mutex);
        }
    }
};

Latch::Latch(size_t n)
    : impl(new Impl(n))
{
}

Latch::Latch(size_t n, CoroExecutor* exec)
    : impl(new Impl(n, exec))
{
}

Latch::~Latch() noexcept {
    delete impl;
}

void Latch::arrive() noexcept {
    impl->arrive();
}

void Latch::wait() noexcept {
    impl->wait();
}
