#include "atomic.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Atomic) {
    STD_TEST(test) {
        int v = 1;

        STD_INSIST(stdAtomicFetch(&v, MemoryOrder::Consume) == 1);
        STD_INSIST(stdAtomicAddAndFetch(&v, 3, MemoryOrder::Acquire) == 4);
        STD_INSIST(stdAtomicSubAndFetch(&v, 2, MemoryOrder::Release) == 2);
        STD_INSIST(stdAtomicFetch(&v, MemoryOrder::Consume) == 2);
    }
}
