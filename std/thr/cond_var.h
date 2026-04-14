#pragma once

namespace stl {
    class Mutex;
    class ObjPool;

    struct CondVarIface;
    struct CoroExecutor;

    class CondVar {
        CondVarIface* impl;

    public:
        CondVar();
        CondVar(CoroExecutor* exec);
        CondVar(CondVarIface* iface);

        static CondVar* create(ObjPool* pool);
        static CondVarIface* createDefault();
        static CondVarIface* createDefault(ObjPool* pool);

        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
