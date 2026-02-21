#include "obj_allocator.h"
#include "mem_pool.h"

#include <std/alg/minmax.h>

using namespace Std;

ObjAllocator::ObjAllocator(size_t objSize, MemoryPool* pool)
    : objSize(max(objSize, sizeof(FreeNode)))
    , pool(pool)
    , freeList(nullptr)
{
}

void* ObjAllocator::allocate() {
    if (freeList) {
        FreeNode* node = freeList;
        freeList = freeList->next;
        return node;
    }

    return pool->allocate(objSize);
}

void ObjAllocator::release(void* ptr) noexcept {
    if (!ptr) {
        return;
    }

    FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
    node->next = freeList;
    freeList = node;
}
