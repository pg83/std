#pragma once

#include <std/sys/types.h>

namespace stl {
    struct FutureIface {
        virtual ~FutureIface() noexcept;

        virtual i32 ref() noexcept = 0;
        virtual i32 unref() noexcept = 0;
        virtual i32 refCount() const noexcept = 0;

        virtual void* wait() noexcept = 0;
        virtual void* posted() noexcept = 0;
        virtual void* release() noexcept = 0;
    };
}
