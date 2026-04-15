#pragma once

#include "runable.h"

namespace stl {
    struct EventIface {
        virtual ~EventIface() noexcept;

        virtual void signal() noexcept = 0;
        virtual void wait(Runable& cb) noexcept = 0;
    };
}
