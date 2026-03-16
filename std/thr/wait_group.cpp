#include "wait_group.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/sys/types.h>
#include <std/sys/atomic.h>

using namespace stl;

struct WaitGroup::Impl {
    Mutex mutex;
    CondVar cv;
    size_t counter;

    Impl(size_t init)
        : counter(init)
    {
    }

    Impl(size_t init, CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
        , counter(init)
    {
    }

    void add(size_t n) noexcept {
        LockGuard lock(mutex);
        counter += n;
    }

    void done() noexcept {
        if (stdAtomicAddAndFetch(&counter, (size_t)-1, MemoryOrder::Release) == 0) {
            LockGuard lock(mutex);
            cv.broadcast();
        }
    }

    void wait() noexcept {
        if (stdAtomicFetch(&counter, MemoryOrder::Acquire) == 0) {
            return;
        }

        LockGuard lock(mutex);

        while (counter > 0) {
            cv.wait(mutex);
        }
    }
};

WaitGroup::WaitGroup()
    : WaitGroup((size_t)0)
{
}

WaitGroup::WaitGroup(CoroExecutor* exec)
    : WaitGroup(0, exec)
{
}

WaitGroup::WaitGroup(size_t init)
    : impl(new Impl(init))
{
}

WaitGroup::WaitGroup(size_t init, CoroExecutor* exec)
    : impl(new Impl(init, exec))
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
