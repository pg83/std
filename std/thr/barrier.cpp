#include "barrier.h"
#include "mutex.h"
#include "cond_var.h"

using namespace stl;

struct Barrier::Impl {
    Mutex mutex;
    CondVar cv;
    int count;
    int total;

    explicit Impl(int n)
        : count(0)
        , total(n)
    {
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
    LockGuard lock(impl->mutex);

    if (++impl->count >= impl->total) {
        impl->cv.broadcast();
    } else {
        while (impl->count < impl->total) {
            impl->cv.wait(impl->mutex);
        }
    }
}
