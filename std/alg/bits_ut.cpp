#include "bits.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(BitOps) {
    STD_TEST(testClp2) {
        STD_INSIST(clp2(0u) == 1);
        STD_INSIST(clp2(1u) == 1);
        STD_INSIST(clp2(2u) == 2);
        STD_INSIST(clp2(3u) == 4);
        STD_INSIST(clp2(4u) == 4);
        STD_INSIST(clp2(5ul) == 8);
        STD_INSIST(clp2(6ul) == 8);
        STD_INSIST(clp2(7ul) == 8);
        STD_INSIST(clp2(8ul) == 8);
    }
}
