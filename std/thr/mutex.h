#pragma once

namespace Std {
    class Mutex {
        alignas(void*) char storage_[64];

    public:
        Mutex();
        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool try_lock() noexcept;
    };

    class LockGuard {
        Mutex& mutex_;

    public:
        explicit LockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~LockGuard() noexcept {
            mutex_.unlock();
        }
    };
}
