#include "pool.h"
#include "mem_pool.h"

#include <std/sys/crt.h>

#include <std/str/view.h>

#include <std/lib/list.h>
#include <std/lib/vector.h>

#include <cmath>
#include <cstddef>

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }

    struct ObjectPool: public Pool, public IntrusiveList, public MemoryPool {
        ~ObjectPool() override {
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

Pool::~Pool() {
}

Pool::Ref Pool::fromMemory() {
    return new ObjectPool();
}

StringView Pool::intern(const StringView& s) {
    auto len = s.length();
    auto res = (u8*)allocate(len);

    memCpy(res, s.data(), len);

    return StringView(res, len);
}

Pool::Dispose::~Dispose() noexcept {
}
