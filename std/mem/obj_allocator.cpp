#include "obj_allocator.h"
#include "mem_pool.h"

#include <std/alg/minmax.h>
#include <std/dbg/assert.h>
#include <std/alg/exchange.h>

using namespace Std;

namespace {
    struct FreeNode {
        FreeNode* next;
    };

    struct alignas(max_align_t) Base: public ObjAllocator {
        MemoryPool mp;
        size_t objSize;
        FreeNode* freeList;

        inline Base(void* buf, size_t len, size_t os)
            : mp(buf, len)
            , freeList(nullptr)
            , objSize(max(os, sizeof(FreeNode)))
        {
        }
    };

    struct Impl: public Base {
        u8 buf[256 - sizeof(Base)];

        inline Impl(size_t os) noexcept
            : Base(buf, sizeof(buf), os)
        {
            STD_ASSERT((size_t)buf % alignof(max_align_t) == 0);
        }

        void* allocate() override {
            if (freeList) {
                return exchange(freeList, freeList->next);
            }

            return mp.allocate(objSize);
        }

        void release(void* ptr) noexcept override {
            auto node = (FreeNode*)ptr;
            node->next = freeList;
            freeList = node;
        }
    };

    static_assert(sizeof(Impl) == 256);
}

ObjAllocator::~ObjAllocator() noexcept {
}

ObjAllocator::Ref ObjAllocator::fromMemory(size_t objSize) {
    return new Impl(objSize);
}
