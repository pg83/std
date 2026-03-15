#include "event.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

using namespace stl;

struct Event::Impl {
    Mutex mutex;
    CondVar cv;
    bool set_ = false;

    Impl() = default;

    Impl(CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
    {
    }

    void set() noexcept {
        LockGuard lock(mutex);

        set_ = true;
        cv.broadcast();
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        while (!set_) {
            cv.wait(mutex);
        }
    }
};

Event::Event()
    : impl(new Impl())
{
}

Event::Event(CoroExecutor* exec)
    : impl(new Impl(exec))
{
}

Event::~Event() noexcept {
    delete impl;
}

void Event::set() noexcept {
    impl->set();
}

void Event::wait() noexcept {
    impl->wait();
}
