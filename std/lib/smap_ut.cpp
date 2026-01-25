#include "smap.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(StringMap) {
    STD_TEST(test1) {
        StringMap<int> s;

        s[u8"qw1"] = 1;
        s[u8"qw2"] = 2;

        STD_INSIST(s[u8"qw1"] == 1);
        STD_INSIST(s[u8"qw2"] == 2);
        STD_INSIST(s[u8"qw3"] == 0);
    }
}
