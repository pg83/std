#include "pcg.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>

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

    STD_TEST(testUniformBiased) {
        i32 cnt[3] = {0};
        PCG32 r(1, 2);

        for (size_t i = 0; i < 2000; ++i) {
            ++cnt[r.uniformBiased(3)];
        }

        STD_INSIST(cnt[0] == 673);
        STD_INSIST(cnt[1] == 655);
        STD_INSIST(cnt[2] == 672);
    }
}
