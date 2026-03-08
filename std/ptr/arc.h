#pragma once

#include <std/sys/types.h>

namespace stl {
    class ARC {
        i32 counter_;

    public:
        ARC();

        i32 ref();
        i32 refCount() const;
        i32 unref();
    };
}
