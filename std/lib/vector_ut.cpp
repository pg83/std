#include "vector.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(vector) {
    STD_TEST(pushBack) {
        Vector<int> vec;

        for (size_t i = 0; i < 1000; ++i) {
            vec.pushBack(i);
        }

        for (size_t i = 0; i < 1000; ++i) {
        }
    }
}
