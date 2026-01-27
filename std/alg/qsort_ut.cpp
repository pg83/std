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
        for (size_t n = 1; n < 500; ++n) {
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

    STD_TEST(EmptyArray) {
        Vector<u32> v;
        quickSort(mutRange(v));
        STD_INSIST(v.empty());
    }

    STD_TEST(SingleElement) {
        Vector<u32> v;
        v.pushBack(42);
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 1);
        STD_INSIST(v[0] == 42);
    }

    STD_TEST(TwoElementsSorted) {
        Vector<u32> v;
        v.pushBack(1);
        v.pushBack(2);
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 2);
        STD_INSIST(v[0] == 1 && v[1] == 2);
    }

    STD_TEST(TwoElementsUnsorted) {
        Vector<u32> v;
        v.pushBack(2);
        v.pushBack(1);
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 2);
        STD_INSIST(v[0] == 1 && v[1] == 2);
    }

    STD_TEST(SmallArray) {
        Vector<u32> v;
        v.pushBack(5);
        v.pushBack(2);
        v.pushBack(8);
        v.pushBack(1);
        v.pushBack(9);
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 5);
        STD_INSIST(v[0] == 1 && v[1] == 2 && v[2] == 5 && v[3] == 8 && v[4] == 9);
    }

    STD_TEST(AlreadySorted) {
        Vector<u32> v;
        for (u32 i = 1; i <= 10; ++i) {
            v.pushBack(i);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 10);
        for (u32 i = 0; i < 10; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(ReverseSorted) {
        Vector<u32> v;
        for (u32 i = 10; i >= 1; --i) {
            v.pushBack(i);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 10);
        for (u32 i = 0; i < 10; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(WithDuplicates) {
        Vector<u32> v;
        v.pushBack(5);
        v.pushBack(2);
        v.pushBack(8);
        v.pushBack(2);
        v.pushBack(9);
        v.pushBack(5);
        v.pushBack(1);
        v.pushBack(8);
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 8);
        STD_INSIST(v[0] == 1);
        STD_INSIST(v[1] == 2 && v[2] == 2);
        STD_INSIST(v[3] == 5 && v[4] == 5);
        STD_INSIST(v[5] == 8 && v[6] == 8);
        STD_INSIST(v[7] == 9);
    }

    STD_TEST(AllSame) {
        Vector<u32> v;
        for (u32 i = 0; i < 7; ++i) {
            v.pushBack(7);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 7);
        for (u32 i = 0; i < 7; ++i) {
            STD_INSIST(v[i] == 7);
        }
    }

    STD_TEST(LargeArray) {
        Vector<u32> v;
        for (u32 i = 0; i < 1000; ++i) {
            v.pushBack(1000 - i);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 1000);
        for (u32 i = 0; i < 1000; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(CustomComparatorDescending) {
        Vector<u32> v;
        v.pushBack(5);
        v.pushBack(2);
        v.pushBack(8);
        v.pushBack(1);
        v.pushBack(9);
        quickSort(mutRange(v), [](const auto& x, const auto& y) {
            return x > y;
        });
        STD_INSIST(v.length() == 5);
        STD_INSIST(v[0] == 9 && v[1] == 8 && v[2] == 5 && v[3] == 2 && v[4] == 1);
    }

    STD_TEST(BoundaryCase15) {
        Vector<u32> v;
        for (u32 i = 15; i >= 1; --i) {
            v.pushBack(i);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 15);
        for (u32 i = 0; i < 15; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(BoundaryCase16) {
        Vector<u32> v;
        for (u32 i = 16; i >= 1; --i) {
            v.pushBack(i);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 16);
        for (u32 i = 0; i < 16; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(BoundaryCase17) {
        Vector<u32> v;
        for (u32 i = 17; i >= 1; --i) {
            v.pushBack(i);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 17);
        for (u32 i = 0; i < 17; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(DuplicatesPattern) {
        Vector<u32> v;
        u32 arr[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
        for (u32 i = 0; i < 11; ++i) {
            v.pushBack(arr[i]);
        }
        quickSort(mutRange(v));
        STD_INSIST(v.length() == 11);
        for (u32 i = 1; i < v.length(); ++i) {
            STD_INSIST(v[i - 1] <= v[i]);
        }
    }
}
