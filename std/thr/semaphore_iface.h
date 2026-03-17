#pragma once

namespace stl {
    struct SemaphoreIface {
        virtual ~SemaphoreIface() noexcept;

        virtual void post() noexcept = 0;
        virtual void wait() noexcept = 0;
    };
}
