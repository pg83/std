#include "wait_group.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

#include <std/sys/types.h>

using namespace stl;

struct WaitGroup::Impl {
    Mutex mutex;
    CondVar cv;
    size_t counter;

    Impl()
        : counter(0)
    {
    }

    Impl(CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
        , counter(0)
    {
    }

    void add(size_t n) noexcept {
        LockGuard lock(mutex);
        counter += n;
    }

    void done() noexcept {
        LockGuard lock(mutex);

        if (--counter == 0) {
            cv.signal();
        }
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        while (counter > 0) {
            cv.wait(mutex);
        }
    }
};

WaitGroup::WaitGroup()
    : impl(new Impl())
{
}

WaitGroup::WaitGroup(CoroExecutor* exec)
    : impl(new Impl(exec))
{
}

WaitGroup::~WaitGroup() noexcept {
    delete impl;
}

void WaitGroup::add(size_t n) noexcept {
    impl->add(n);
}

void WaitGroup::done() noexcept {
    impl->done();
}

void WaitGroup::wait() noexcept {
    impl->wait();
}
