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

        static Mutex* createDefault(ObjPool* pool);
        static Mutex* createSpinLock(ObjPool* pool);
        static Mutex* createSpinLock(ObjPool* pool, CoroExecutor* exec);

        static SemaphoreIface* spinLock(CoroExecutor* exec);
        static SemaphoreIface* spinLock(ObjPool* pool, CoroExecutor* exec);

        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool tryLock() noexcept;

        void* nativeHandle() noexcept;
    };
}
