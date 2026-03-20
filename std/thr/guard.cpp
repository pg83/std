#include "guard.h"
#include "mutex.h"

using namespace stl;

LockGuard::LockGuard(Mutex& mutex)
    : mutex_(&mutex)
{
    mutex_->lock();
}

LockGuard::~LockGuard() noexcept {
    if (mutex_) {
        mutex_->unlock();
    }
}

UnlockGuard::UnlockGuard(Mutex& mutex)
    : mutex_(mutex)
{
    mutex_.unlock();
}

UnlockGuard::~UnlockGuard() noexcept {
    mutex_.lock();
}
