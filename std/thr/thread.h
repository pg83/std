#pragma once

namespace Std {
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
    };

    class ScopedThread {
        Thread thr;

    public:
        inline ScopedThread(Runable& runable)
            : thr(runable)
        {
        }

        inline ~ScopedThread() noexcept {
            thr.join();
        }
    };

    void detach(Runable& runable);
}
