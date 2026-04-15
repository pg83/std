#pragma once

namespace stl {
    class ObjPool;

    struct Mutex;
    struct CoroExecutor;

    struct CondVar {
        virtual void wait(Mutex& mutex) noexcept = 0;
        virtual void signal() noexcept = 0;
        virtual void broadcast() noexcept = 0;

        static CondVar* create(ObjPool* pool);
        static CondVar* create(ObjPool* pool, CoroExecutor* exec);
    };
}
