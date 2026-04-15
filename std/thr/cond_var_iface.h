#pragma once

namespace stl {
    class Mutex;

    struct CondVarIface {
        virtual ~CondVarIface() noexcept;

        virtual void wait(Mutex& mutex) noexcept = 0;
        virtual void signal() noexcept = 0;
        virtual void broadcast() noexcept = 0;

        virtual bool owned() const noexcept;
    };
}
