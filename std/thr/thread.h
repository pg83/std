#pragma once

namespace Std {
    struct Runable {
        virtual void run() = 0;
    };

    class Thread {
        alignas(void*) char storage_[64];

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
