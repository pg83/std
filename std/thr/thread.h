#pragma once

#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;
    struct ThreadIface;

    // should not be used directly, use ScopedThread or detach(Runable&)
    class Thread {
        ThreadIface* impl;

    public:
        explicit Thread(Runable& runable);
        explicit Thread(ThreadIface* iface);
        Thread(CoroExecutor* exec, Runable& runable);

        ~Thread() noexcept;

        void join() noexcept;
        void detach() noexcept;

        u64 threadId() const noexcept;

        static u64 currentThreadId() noexcept;
    };

    class ScopedThread {
        Thread thr;

    public:
        template <typename T>
        ScopedThread(T functor)
            : thr(*makeRunablePtr(functor))
        {
        }

        ~ScopedThread() noexcept {
            thr.join();
        }
    };

    void detach(Runable& runable);
}
