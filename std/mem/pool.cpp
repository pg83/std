#include "pool.h"
#include "alloc.h"

#include <std/alg/range.h>
#include <std/lib/vector.h>

using namespace Std;

namespace {
    struct DebugPool: public Pool, public Vector<void*> {
        ~DebugPool() {
            for (auto ptr : mutRange(*this)) {
                freeMemory(ptr);
            }
        }

        void* allocate(size_t len) override {
            pushBack(allocateMemory(len));

            return back();
        }
    };
}

Pool::Ref Pool::fromMemory() {
    return new DebugPool();
}
