#include "free_list.h"
#include "mem_pool.h"
#include "obj_pool.h"

#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    struct Node {
        Node* next;
    };

    struct Impl: public FreeList {
        MemoryPool* mp;
        size_t objSize;
        Node* freeList;

        Impl(MemoryPool* mp, size_t os) noexcept
            : mp(mp)
            , objSize(max(os, sizeof(Node)))
            , freeList(nullptr)
        {
        }

        void* allocate() override {
            if (freeList) {
                return exchange(freeList, freeList->next);
            }

            return mp->allocate(objSize);
        }

        void release(void* ptr) noexcept override {
            auto node = (Node*)ptr;
            node->next = freeList;
            freeList = node;
        }
    };
}

FreeList::~FreeList() noexcept {
}

FreeList* FreeList::create(ObjPool* pool, size_t objSize) {
    return pool->make<Impl>(pool->memoryPool(), objSize);
}
