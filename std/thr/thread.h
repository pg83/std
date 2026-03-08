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
        ~Thread();

        void join();
        void detach();

        u64 threadId() const;

        static u64 currentThreadId();
    };

    class ScopedThread {
        Thread thr;

    public:
        ScopedThread(Runable& runable)
            : thr(runable)
        {
        }

        ~ScopedThread() {
            thr.join();
        }
    };

    void detach(Runable& runable);
}
