#pragma once

namespace stl {
    struct SemaphoreIface {
        virtual ~SemaphoreIface() noexcept;

        virtual void post() noexcept = 0;
        virtual void wait() noexcept = 0;
        virtual bool tryWait() noexcept = 0;

        virtual void* nativeHandle() noexcept;
        virtual bool owned() const noexcept;
    };
}
