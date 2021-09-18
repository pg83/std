#include "range.h"
#include "reverse.h"

#include <std/tst/ut.h>
#include <std/lib/vector.h>

using namespace Std;

STD_TEST_SUITE(reverse) {
    STD_TEST(test1) {
        Vector<u32> x;

        reverse(mutRange(x));

        STD_INSIST(x.empty());
    }
}
