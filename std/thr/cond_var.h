#pragma once

namespace stl {
    class Mutex;

    struct CondVarIface;
    struct CoroExecutor;

    class CondVar {
        CondVarIface* impl;

    public:
        CondVar();
        CondVar(CoroExecutor* exec);
        CondVar(CondVarIface* iface);

        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
