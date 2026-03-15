#pragma once

namespace stl {
    struct EventIface {
        virtual ~EventIface() noexcept;

        virtual void set() noexcept = 0;
        virtual void wait() noexcept = 0;
    };
}
