#include "i_map.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(IntMap) {
    STD_TEST(Simple) {
        IntMap<int> m;

        m[0] = 0;
        m[1] = 1;

        STD_INSIST(m[0] == 0);
        STD_INSIST(m[1] == 1);
    }
}
