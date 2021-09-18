#include "vector.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(vector) {
    STD_TEST(pushBack) {
        Vector<size_t> vec;

        for (size_t i = 0; i < 1000; ++i) {
            vec.pushBack(i);
        }

        for (size_t i = 0; i < 1000; ++i) {
            STD_INSIST(vec[i] == i);
        }

        STD_INSIST(vec.length() == 1000);
        STD_INSIST(!vec.empty());
    }
}
