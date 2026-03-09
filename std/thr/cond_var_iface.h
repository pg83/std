#pragma once

namespace stl {
    struct MutexIface;

    struct CondVarIface {
        virtual ~CondVarIface() noexcept;

        virtual void wait(MutexIface* mutex) noexcept = 0;
        virtual void signal() noexcept = 0;
        virtual void broadcast() noexcept = 0;
    };
}
