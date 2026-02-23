#include "free_list.h"
#include "mem_pool.h"

#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace Std;

namespace {
    struct Node {
        Node* next;
    };

    struct alignas(max_align_t) Base: public FreeList {
        MemoryPool mp;
        size_t objSize;
        Node* freeList;

        inline Base(void* buf, size_t len, size_t os)
            : mp(buf, len)
            , freeList(nullptr)
            , objSize(max(os, sizeof(Node)))
        {
        }
    };

    struct Impl: public Base {
        alignas(max_align_t) u8 buf[256 - sizeof(Base)];

        inline Impl(size_t os) noexcept
            : Base(buf, sizeof(buf), os)
        {
        }

        void* allocate() override {
            if (freeList) {
                return exchange(freeList, freeList->next);
            }

            return mp.allocate(objSize);
        }

        void release(void* ptr) noexcept override {
            auto node = (Node*)ptr;
            node->next = freeList;
            freeList = node;
        }
    };

    static_assert(sizeof(Impl) == 256);
    static_assert(sizeof(Base) % sizeof(max_align_t) == 0);
}

FreeList::~FreeList() noexcept {
}

FreeList::Ref FreeList::fromMemory(size_t objSize) {
    return new Impl(objSize);
}
