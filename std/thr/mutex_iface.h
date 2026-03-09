#pragma once

namespace stl {
    struct MutexIface {
        virtual ~MutexIface() noexcept;

        virtual void lock() noexcept = 0;
        virtual void unlock() noexcept = 0;
        virtual bool tryLock() noexcept = 0;

        virtual void* nativeHandle() noexcept;
    };
}
