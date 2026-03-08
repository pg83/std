#pragma once

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class FreeList: public ARC {
    public:
        using Ref = IntrusivePtr<FreeList>;

        virtual ~FreeList();

        virtual void* allocate() = 0;
        virtual void release(void* ptr) = 0;

        static Ref fromMemory(size_t objSize);
    };
}
