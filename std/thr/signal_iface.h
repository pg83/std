#pragma once

namespace stl {
    struct SignalIface {
        virtual ~SignalIface() noexcept;

        virtual void set() noexcept = 0;
        virtual void wait() noexcept = 0;
    };
}
