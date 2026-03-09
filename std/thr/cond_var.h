#pragma once

namespace stl {
    class Mutex;
    struct CondVarIface;
    struct CoroExecutor;

    class CondVar {
        CondVarIface* impl;

    public:
        CondVar();
        CondVar(CondVarIface* iface);
        CondVar(CoroExecutor* exec);
        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
