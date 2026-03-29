#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    class FreeList {
    public:
        virtual ~FreeList() noexcept;

        virtual void* allocate() = 0;
        virtual void release(void* ptr) noexcept = 0;

        static FreeList* create(ObjPool* pool, size_t objSize);
    };
}
