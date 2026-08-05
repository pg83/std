#include "bound.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(Bound) {
    static const int sorted[] = {1, 3, 3, 5, 9};
    static const int* const b = sorted;
    static const int* const e = sorted + 5;

    STD_TEST(testLowerBound) {
        STD_INSIST(lowerBound(b, e, 0) == b);
        STD_INSIST(lowerBound(b, e, 1) == b);
        STD_INSIST(lowerBound(b, e, 2) == b + 1);
        STD_INSIST(lowerBound(b, e, 3) == b + 1);
        STD_INSIST(lowerBound(b, e, 4) == b + 3);
        STD_INSIST(lowerBound(b, e, 9) == b + 4);
        STD_INSIST(lowerBound(b, e, 10) == e);
        STD_INSIST(lowerBound(b, b, 1) == b);
    }

    STD_TEST(testUpperBound) {
        STD_INSIST(upperBound(b, e, 0) == b);
        STD_INSIST(upperBound(b, e, 1) == b + 1);
        STD_INSIST(upperBound(b, e, 3) == b + 3);
        STD_INSIST(upperBound(b, e, 9) == e);
        STD_INSIST(upperBound(b, b, 1) == b);
    }

    STD_TEST(testBinaryContains) {
        STD_INSIST(binaryContains(b, e, 1));
        STD_INSIST(binaryContains(b, e, 3));
        STD_INSIST(binaryContains(b, e, 9));
        STD_INSIST(!binaryContains(b, e, 0));
        STD_INSIST(!binaryContains(b, e, 4));
        STD_INSIST(!binaryContains(b, e, 10));
        STD_INSIST(!binaryContains(b, b, 1));
    }
}
