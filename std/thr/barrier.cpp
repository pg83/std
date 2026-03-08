#include "barrier.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/sys/types.h>

using namespace stl;

struct Barrier::Impl {
    Mutex mutex;
    CondVar cv;
    size_t remaining;

    Impl(int n)
        : remaining(n)
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

Barrier::Barrier(int n)
    : impl(new Impl(n))
{
}

Barrier::~Barrier() noexcept {
    delete impl;
}

void Barrier::wait() noexcept {
    impl->wait();
}
