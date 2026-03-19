#pragma once

namespace stl {
    struct SemaphoreIface;

    struct CondVarIface {
        virtual ~CondVarIface() noexcept;

        virtual void wait(SemaphoreIface* mutex) noexcept = 0;
        virtual void signal() noexcept = 0;
        virtual void broadcast() noexcept = 0;
    };
}
