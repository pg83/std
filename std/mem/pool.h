#pragma once

#include <std/sys/types.h>

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace Std {
    class StringView;

    struct Pool: public ARC {
        using Ref = IntrusivePtr<Pool>;

        virtual ~Pool();

        virtual void* allocate(size_t len) = 0;

        StringView intern(const StringView& s);

        static Ref fromMemory();
    };
}
