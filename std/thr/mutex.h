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
}
