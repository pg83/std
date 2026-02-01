#pragma once

namespace Std {
    struct Runable {
        virtual void run() = 0;
    };

    // should not be used directly, use ScopedThread or detach(Runable&)
    class Thread {
        alignas(void*) char storage_[16];

        struct Impl;
        Impl* impl() const noexcept;

    public:
        explicit Thread(Runable& runable);
        ~Thread() noexcept;

        void join();
        void detach();
    };

    class ScopedThread {
        Thread thr;

    public:
        inline ScopedThread(Runable& runable)
            : thr(runable)
        {
        }

        inline ~ScopedThread() {
            thr.join();
        }
    };

    void detach(Runable& runable);
}
