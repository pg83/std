#include "smap.h"

#include <std/tst/ut.h>
#include <std/lib/vector.h>

using namespace Std;

STD_TEST_SUITE(StringMap) {
    STD_TEST(test1) {
        StringMap<int> s;

        s["qw1"] = 1;
        s["qw2"] = 2;

        STD_INSIST(s["qw1"] == 1);
        STD_INSIST(s["qw2"] == 2);
        STD_INSIST(s["qw3"] == 0);
    }

    STD_TEST(test2) {
        StringMap<Vector<int>> s;

        s["qw1"].pushBack(1);
        s["qw1"].pushBack(2);
        s["qw2"].pushBack(3);
        s["qw2"].pushBack(4);

        STD_INSIST(s["qw1"][0] == 1);
        STD_INSIST(s["qw1"][1] == 2);
        STD_INSIST(s["qw2"][0] == 3);
        STD_INSIST(s["qw2"][1] == 4);
    }
}
