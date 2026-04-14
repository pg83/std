#pragma once

namespace stl {
    class ObjPool;

    struct CoroExecutor;
    struct SemaphoreIface;

    class Mutex {
        SemaphoreIface* impl;

    public:
        Mutex();
        Mutex(bool lock);
        Mutex(CoroExecutor* exec);
        Mutex(SemaphoreIface* iface);

        static Mutex* createDefault(ObjPool*);
        static Mutex* createSpinLock(ObjPool*, CoroExecutor*);

        static SemaphoreIface* spinLock(CoroExecutor*);
        static SemaphoreIface* spinLock(ObjPool*, CoroExecutor*);

        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool tryLock() noexcept;

        void* nativeHandle() noexcept;
    };
}
