#pragma once

namespace stl {
    struct SpinLock {
        char flag_ = 0;
        void lock() noexcept;
        void unlock() noexcept;
    };
}
