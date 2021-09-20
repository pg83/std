#include "qsort.h"
#include "range.h"

#include <std/tst/ut.h>
#include <std/rng/pcg.h>
#include <std/lib/vector.h>

using namespace Std;

STD_TEST_SUITE(QuickSort) {
    STD_TEST(test0) {
        Vector<u32> v;

        quickSort(mutRange(v));

        STD_INSIST(v.empty());
    }

    STD_TEST(test1) {
        Vector<u32> v;

        v.pushBack(1);

        quickSort(mutRange(v));

        STD_INSIST(v.length() == 1);
        STD_INSIST(v[0] == 1);
    }

    STD_TEST(test2) {
        Vector<u32> v;

        v.pushBack(2);
        v.pushBack(1);

        quickSort(mutRange(v));

        STD_INSIST(v.length() == 2);
        STD_INSIST(v[0] == 1);
        STD_INSIST(v[1] == 2);
    }

    STD_TEST(test10) {
        Vector<u32> v;

        for (size_t i = 0; i < 10; ++i) {
            v.pushBack(9 - i);
        }

        quickSort(mutRange(v));

        for (size_t i = 0; i < 10; ++i) {
            STD_INSIST(v[i] == i);
        }
    }

    STD_TEST(testStress) {
        for (size_t n = 1; n < 1000; ++n) {
            Vector<u32> v;
            PCG32 r(n);

            for (size_t i = 0; i < n; ++i) {
                v.pushBack(r.nextU32());
            }

            quickSort(mutRange(v));

            for (size_t i = 1; i < v.length(); ++i) {
                STD_INSIST(v[i - 1] <= v[i]);
            }
        }
    }
}
