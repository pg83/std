#pragma once

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace Std {
    class FreeList: public ARC {
    public:
        using Ref = IntrusivePtr<FreeList>;

        virtual ~FreeList() noexcept;

        virtual void* allocate() = 0;
        virtual void release(void* ptr) noexcept = 0;

        static Ref fromMemory(size_t objSize);
    };
}
