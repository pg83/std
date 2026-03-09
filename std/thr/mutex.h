#pragma once

namespace stl {
    class CondVar;

    struct MutexIface;
    struct CoroExecutor;

    class Mutex {
        friend class CondVar;

        MutexIface* impl;

    public:
        Mutex();
        Mutex(bool lock);
        Mutex(MutexIface* iface);
        Mutex(CoroExecutor* exec);

        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool tryLock() noexcept;
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

    class UnlockGuard {
        Mutex& mutex_;

    public:
        explicit UnlockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.unlock();
        }

        ~UnlockGuard() noexcept {
            mutex_.lock();
        }
    };
}
