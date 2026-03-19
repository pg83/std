#include "spin_lock.h"

using namespace stl;

void SpinLock::lock() noexcept {
    while (__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE)) {
    }
}

void SpinLock::unlock() noexcept {
    __atomic_clear(&flag_, __ATOMIC_RELEASE);
}
