#include "pool.h"

#include <std/sys/crt.h>

#include <std/str/view.h>

#include <std/alg/range.h>
#include <std/alg/reverse.h>

#include <std/lib/vector.h>

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }

    struct DebugPool: public Pool {
        Vector<void*> mem;
        Vector<Dispose*> obj;

        ~DebugPool() {
            reverse(mutRange(obj));

            for (auto ptr : mutRange(obj)) {
                destruct(ptr);
            }

            for (auto ptr : mutRange(mem)) {
                freeMemory(ptr);
            }
        }

        void* allocate(size_t len) override {
            mem.pushBack(allocateMemory(len));

            return mem.back();
        }

        void submit(Dispose* d) noexcept override {
            obj.pushBack(d);
        }
    };
}

Pool::~Pool() {
}

Pool::Ref Pool::fromMemory() {
    return new DebugPool();
}

StringView Pool::intern(const StringView& s) {
    auto len = s.length();
    auto res = (u8*)allocate(len);

    memCpy(res, s.data(), len);

    return StringView(res, len);
}

Pool::Dispose::~Dispose() {
}
