#include "pool.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(MemoryPool) {
    STD_TEST(stress) {
        auto pool = Pool::fromMemory();

        for (size_t i = 0; i < 1000; ++i) {
            pool->allocate(i);
        }
    }

    STD_TEST(obj) {
        struct T {
            u64* ptr;

            inline T(u64* _ptr) noexcept
                : ptr(_ptr)
            {
            }

            inline ~T() {
                *ptr = 0;
            }
        };

        u64 v = 1;

        {
            auto pool = Pool::fromMemory();
            auto obj = pool->make<T>(&v);

            STD_INSIST(v == 1);
            STD_INSIST(obj->ptr == &v);
            STD_INSIST(*obj->ptr == 1);
        }

        STD_INSIST(v == 0);
    }
}
