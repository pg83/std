#include "spin_lock.h"
#include "coro.h"

#include <sched.h>

using namespace stl;

void SpinLock::lock() noexcept {
    while (__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE)) {
        if (0) {
            sched_yield();
        } else if (exec_ && exec_->me()) {
            exec_->yield();
        } else {
            sched_yield();
        }
    }
}

void SpinLock::unlock() noexcept {
    __atomic_clear(&flag_, __ATOMIC_RELEASE);
}
