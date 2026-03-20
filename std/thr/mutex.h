#pragma once

namespace stl {
    struct CoroExecutor;
    struct SemaphoreIface;

    class Mutex {
        SemaphoreIface* impl;

    public:
        Mutex();
        Mutex(bool lock);
        Mutex(CoroExecutor* exec);
        Mutex(SemaphoreIface* iface);

        static SemaphoreIface* spinLock(CoroExecutor*);

        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool tryLock() noexcept;

        void* nativeHandle() noexcept;
    };

    class LockGuard {
        Mutex* mutex_;

    public:
        explicit LockGuard(Mutex& mutex)
            : mutex_(&mutex)
        {
            mutex_->lock();
        }

        ~LockGuard() noexcept {
            if (mutex_) {
                mutex_->unlock();
            }
        }

        template <typename F>
        auto run(F f) {
            return f();
        }

        void drop() noexcept {
            mutex_ = nullptr;
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

        template <typename F>
        auto run(F f) {
            return f();
        }
    };
}
