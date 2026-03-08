#pragma once

namespace stl {
    class CondVar;

    class Mutex {
        friend class CondVar;

        struct Impl;
        Impl* impl;

    public:
        Mutex();
        Mutex(bool lock);
        ~Mutex();

        void lock();
        void unlock();
        bool tryLock();
    };

    class LockGuard {
        Mutex& mutex_;

    public:
        explicit LockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~LockGuard() {
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

        ~UnlockGuard() {
            mutex_.lock();
        }
    };
}
