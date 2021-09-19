#include "bits.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(BitOps) {
    STD_TEST(testClp2) {
        STD_INSIST(clp2(0) == 1);
        STD_INSIST(clp2(1) == 1);
        STD_INSIST(clp2(2) == 2);
        STD_INSIST(clp2(3) == 4);
        STD_INSIST(clp2(4) == 4);
        STD_INSIST(clp2(5) == 8);
        STD_INSIST(clp2(6) == 8);
        STD_INSIST(clp2(7) == 8);
        STD_INSIST(clp2(8) == 8);
    }
}
