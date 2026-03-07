#include "barrier.h"
#include "mutex.h"
#include "cond_var.h"

using namespace stl;

struct Barrier::Impl {
    Mutex mutex;
    CondVar cv;
    int count;
    int total;

    Impl(int n)
        : count(0)
        , total(n)
    {
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        if (++count >= total) {
            cv.broadcast();
        } else {
            while (count < total) {
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
