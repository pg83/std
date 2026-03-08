#include "latch.h"
#include "mutex.h"
#include "cond_var.h"

using namespace stl;

struct Latch::Impl {
    Mutex mutex;
    CondVar cv;
    int target;
    int count = 0;

    Impl(int n)
        : target(n)
    {
    }

    void arrive() noexcept {
        LockGuard lock(mutex);

        if (++count == target) {
            cv.signal();
        }
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        while (count < target) {
            cv.wait(mutex);
        }
    }
};

Latch::Latch(int n)
    : impl(new Impl(n))
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
