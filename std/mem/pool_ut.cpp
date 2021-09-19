#include "pool.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(MemoryPool) {
    STD_TEST(stress) {
        Pool::Ref pool = Pool::fromMemory();

        for (size_t i = 0; i < 1000; ++i) {
            pool->allocate(i);
        }
    }
}
