#pragma once

#include <std/sys/types.h>

#include <std/ptr/ref_count.h>
#include <std/ptr/intrusive.h>

namespace Std {
    struct Pool: public AtomicRefCount {
        using Ref = IntrusivePtr<Pool>;

        virtual ~Pool();

        virtual void* allocate(size_t len) = 0;

        static Ref fromMemory();
    };
}
