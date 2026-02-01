#pragma once

namespace Std {
    class CondVar;

    class Mutex {
        friend class CondVar;

        alignas(void*) char storage_[128];
        struct Impl;
        Impl* impl() noexcept;

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

    class CondVar {
        alignas(void*) char storage_[128];
        struct Impl;
        Impl* impl() noexcept;

    public:
        CondVar();
        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
