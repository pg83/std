#include "range.h"
#include "reverse.h"

#include <std/tst/ut.h>
#include <std/lib/vector.h>

using namespace Std;

STD_TEST_SUITE(Reverse) {
    STD_TEST(testEmpty) {
        Vector<u32> x;

        reverse(mutRange(x));

        STD_INSIST(x.empty());
    }

    STD_TEST(testOne) {
        Vector<u16> x;

        x.pushBack(1);

        reverse(mutRange(x));

        STD_INSIST(x.length() == 1);
        STD_INSIST(x[0] == 1);
    }

    STD_TEST(testTwo) {
        Vector<u64> x;

        x.pushBack(1);
        x.pushBack(2);

        reverse(mutRange(x));

        STD_INSIST(x.length() == 2);
        STD_INSIST(x[0] == 2);
        STD_INSIST(x[1] == 1);
    }

    STD_TEST(testThree) {
        Vector<u8> x;

        x.pushBack(1);
        x.pushBack(2);
        x.pushBack(3);

        reverse(mutRange(x));

        STD_INSIST(x.length() == 3);
        STD_INSIST(x[0] == 3);
        STD_INSIST(x[1] == 2);
        STD_INSIST(x[2] == 1);
    }
}
