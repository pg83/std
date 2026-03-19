#pragma once

namespace stl {
    struct SpinLock {
        char flag_ = 0;
        void lock() noexcept { while (__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE)) {} }
        void unlock() noexcept { __atomic_clear(&flag_, __ATOMIC_RELEASE); }
    };
}
