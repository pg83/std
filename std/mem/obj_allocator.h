#pragma once

#include <std/sys/types.h>

namespace Std {
    class MemoryPool;

    class ObjAllocator {
        struct FreeNode {
            FreeNode* next;
        };

        size_t objSize;
        MemoryPool* pool;
        FreeNode* freeList;

    public:
        ObjAllocator(size_t objSize, MemoryPool* pool);

        void* allocate();
        void release(void* ptr) noexcept;
    };
}
