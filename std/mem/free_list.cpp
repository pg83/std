#include "free_list.h"
#include "obj_pool.h"

#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    struct Node {
        Node* next;
    };

    struct Impl: public FreeList {
        ObjPool* pool;
        size_t objSize;
        Node* freeList;

        Impl(ObjPool* p, size_t os) noexcept
            : pool(p)
            , objSize(max(os, sizeof(Node)))
            , freeList(nullptr)
        {
        }

        void* allocate() override {
            if (freeList) {
                return exchange(freeList, freeList->next);
            }

            return pool->allocate(objSize);
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
    return pool->make<Impl>(pool, objSize);
}
