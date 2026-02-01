#pragma once

namespace Std {
    class Mutex {
        alignas(void*) char storage_[128];

    public:
        Mutex();
        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool tryLock() noexcept;
    };

    class LockGuard {
        Mutex& mutex_;

    public:
        inline explicit LockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.lock();
        }

        inline ~LockGuard() noexcept {
            mutex_.unlock();
        }
    };
}
