#include "obj_pool.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>
#include <std/str/view.h>

using namespace stl;

namespace {
    struct alignas(max_align_t) Base: public ObjPool {
        MemoryPool mp;
        Disposer ds;

        Base(void* buf, size_t len)
            : mp(buf, len)
        {
        }
    };

    struct Pool: public Base {
        alignas(max_align_t) u8 buf[256 - sizeof(Base)];

        Pool() noexcept
            : Base(buf, sizeof(buf))
        {
        }

        void* allocate(size_t len) override {
            return mp.allocate(len);
        }

        void submit(Disposable* d) noexcept override {
            ds.submit(d);
        }

        MemoryPool* memoryPool() noexcept override {
            return &mp;
        }
    };

    static_assert(sizeof(Pool) == 256);
}

ObjPool::~ObjPool() noexcept {
}

void* ObjPool::allocateOverAligned(size_t len, size_t align) {
    auto raw = (uintptr_t)allocate(len + align);

    return (void*)((raw + align - 1) & ~(align - 1));
}

ObjPool* ObjPool::create(ObjPool* pool) {
    return pool->make<Pool>();
}

ObjPool* ObjPool::fromMemoryRaw() {
    return new Pool();
}

StringView ObjPool::intern(StringView s) {
    auto len = s.length();
    auto res = (u8*)allocate(len + 1);

    *(u8*)memCpy(res, s.data(), len) = 0;

    return StringView(res, len);
}
