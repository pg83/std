#pragma once

namespace Std {
    class Mutex;

    class CondVar {
        alignas(void*) char storage_[128];

        struct Impl;
        Impl* impl() noexcept;

    public:
        CondVar();
        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
