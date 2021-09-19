#pragma once

#include <std/sys/types.h>

namespace Std {
    class ARC {
        i32 counter_;

    public:
        ARC() noexcept;

        i32 ref() noexcept;
        i32 refCount() const noexcept;
        i32 unref() noexcept;
    };
}
