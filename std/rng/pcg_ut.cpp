#include "pcg.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(RNG) {
    STD_TEST(test32Stability) {
        PCG32 r(42, 54);

        STD_INSIST(r.nextU32() == 0xa15c02b7);
        STD_INSIST(r.nextU32() == 0x7b47f409);
        STD_INSIST(r.nextU32() == 0xba1d3330);
        STD_INSIST(r.nextU32() == 0x83d2f293);
        STD_INSIST(r.nextU32() == 0xbfa4784b);
        STD_INSIST(r.nextU32() == 0xcbed606e);
    }
}
