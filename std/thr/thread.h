#pragma once

namespace Std {
    struct Runnable {
        virtual void run() = 0;
    };

    class Thread {
        alignas(void*) char storage_[64];

        struct Impl;
        Impl* impl() const noexcept;

    public:
        explicit Thread(Runnable& runnable);
        ~Thread() noexcept;

        void join();
        void detach();
    };
}
