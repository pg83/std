#include "obj_pool.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>

#include <std/str/view.h>

using namespace Std;

namespace {
    struct Pool: public ObjPool {
        MemoryPool mp;
        Disposer ds;

        void* allocate(size_t len) override {
            return mp.allocate(len);
        }

        void submit(Disposable* d) noexcept override {
            ds.submit(d);
        }
    };
}

ObjPool::~ObjPool() noexcept {
}

ObjPool::Ref ObjPool::fromMemory() {
    return new Pool();
}

StringView ObjPool::intern(const StringView& s) {
    auto len = s.length();
    auto res = (u8*)allocate(len + 1);

    *(u8*)memCpy(res, s.data(), len) = 0;

    return StringView(res, len);
}
