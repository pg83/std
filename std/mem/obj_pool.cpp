#include "obj_pool.h"
#include "mem_pool.h"

#include <std/sys/crt.h>

#include <std/str/view.h>

#include <std/lib/list.h>
#include <std/lib/vector.h>

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }

    struct Pool: public ObjPool, public IntrusiveList, public MemoryPool {
        ~Pool() noexcept override {
            while (!empty()) {
                destruct((Dispose*)popBack());
            }
        }

        void* allocate(size_t len) override {
            return MemoryPool::allocate(len);
        }

        void submit(Dispose* d) noexcept override {
            pushBack(d);
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
    auto res = (u8*)allocate(len);

    memCpy(res, s.data(), len);

    return StringView(res, len);
}

ObjPool::Dispose::~Dispose() noexcept {
}
