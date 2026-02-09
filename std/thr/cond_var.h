#pragma once

namespace Std {
    class Mutex;

    class CondVar {
        struct Impl;
        Impl* impl;

    public:
        CondVar();
        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
