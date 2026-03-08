#pragma once

#include <std/sys/types.h>

namespace stl {
    struct Runable;

    // should not be used directly, use ScopedThread or detach(Runable&)
    class Thread {
        struct Impl;
        Impl* impl;

    public:
        explicit Thread(Runable& runable);
        ~Thread() noexcept;

        void join() noexcept;
        void detach() noexcept;

        u64 threadId() const noexcept;

        static u64 currentThreadId() noexcept;
    };

    class ScopedThread {
        Thread thr;

    public:
        ScopedThread(Runable& runable)
            : thr(runable)
        {
        }

        ~ScopedThread() noexcept {
            thr.join();
        }
    };

    void detach(Runable& runable);
}
