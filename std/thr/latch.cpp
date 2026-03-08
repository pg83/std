#include "latch.h"
#include "mutex.h"
#include "cond_var.h"

using namespace stl;

struct Latch::Impl {
    Mutex mutex;
    CondVar cv;
    size_t remaining;

    Impl(size_t n)
        : remaining(n)
    {
    }

    void arrive() noexcept {
        LockGuard lock(mutex);

        if (--remaining == 0) {
            cv.signal();
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

Latch::~Latch() noexcept {
    delete impl;
}

void Latch::arrive() noexcept {
    impl->arrive();
}

void Latch::wait() noexcept {
    impl->wait();
}
