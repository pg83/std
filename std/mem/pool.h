#pragma once

#include <std/lib/ptr.h>
#include <std/sys/types.h>

namespace Std {
    struct Pool: public RefCount {
        using Ref = IntrusivePtr<Pool>;

        virtual ~Pool();

        virtual void* allocate(size_t len) = 0;

        static Ref fromMemory();
    };
}
