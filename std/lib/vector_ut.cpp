#include "vector.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Vector) {
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

    STD_TEST(popBack) {
        Vector<u32> v;

        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);

        STD_INSIST(v.popBack() == 3);
        STD_INSIST(v.length() == 2);
        STD_INSIST(v.popBack() == 2);
        STD_INSIST(v.length() == 1);
        STD_INSIST(v.popBack() == 1);
        STD_INSIST(v.length() == 0);
    }
}
