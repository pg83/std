#include "obj_pool.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/dbg/assert.h>

using namespace Std;

namespace {
    struct alignas(max_align_t) Base: public ObjPool {
        MemoryPool mp;
        Disposer ds;

        inline Base(void* buf, size_t len)
            : mp(buf, len)
        {
        }
    };

    struct Pool: public Base {
        u8 buf[256 - sizeof(Base)];

        inline Pool() noexcept
            : Base(buf, sizeof(buf))
        {
            STD_ASSERT((size_t)buf % alignof(max_align_t) == 0);
        }

        void* allocate(size_t len) override {
            return mp.allocate(len);
        }

        void submit(Disposable* d) noexcept override {
            ds.submit(d);
        }
    };

    static_assert(sizeof(Pool) == 256);
}

ObjPool::~ObjPool() noexcept {
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
