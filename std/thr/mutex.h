#pragma once

namespace Std {
    class CondVar;

    class Mutex {
        friend class CondVar;

        struct Impl;
        Impl* impl;

    public:
        Mutex();
        Mutex(bool lock);
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

    class UnlockGuard {
        Mutex& mutex_;

    public:
        inline explicit UnlockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.unlock();
        }

        inline ~UnlockGuard() noexcept {
            mutex_.lock();
        }
    };
}
