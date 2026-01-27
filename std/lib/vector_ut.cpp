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

    STD_TEST(DefaultConstruction) {
        Vector<u32> v;
        STD_INSIST(v.empty());
        STD_INSIST(v.length() == 0);
        STD_INSIST(v.begin() == v.end());
    }

    STD_TEST(PushBackSingle) {
        Vector<u32> v;
        v.pushBack(42);
        STD_INSIST(!v.empty());
        STD_INSIST(v.length() == 1);
        STD_INSIST(v[0] == 42);
        STD_INSIST(v.back() == 42);
    }

    STD_TEST(PushBackMultiple) {
        Vector<u32> v;
        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        STD_INSIST(v.length() == 3);
        STD_INSIST(v[0] == 1);
        STD_INSIST(v[1] == 2);
        STD_INSIST(v[2] == 3);
    }

    STD_TEST(PopBack) {
        Vector<u32> v;
        v.pushBack(10);
        v.pushBack(20);
        v.pushBack(30);
        auto val = v.popBack();
        STD_INSIST(val == 30);
        STD_INSIST(v.length() == 2);
        STD_INSIST(v.back() == 20);
    }

    STD_TEST(PopBackToEmpty) {
        Vector<u32> v;
        v.pushBack(42);
        auto val = v.popBack();
        STD_INSIST(val == 42);
        STD_INSIST(v.empty());
        STD_INSIST(v.length() == 0);
    }

    STD_TEST(Clear) {
        Vector<u32> v;
        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        v.clear();
        STD_INSIST(v.empty());
        STD_INSIST(v.length() == 0);
    }

    STD_TEST(ClearAndReuse) {
        Vector<u32> v;
        v.pushBack(1);
        v.clear();
        v.pushBack(2);
        STD_INSIST(v.length() == 1);
        STD_INSIST(v[0] == 2);
    }

    STD_TEST(MutAccess) {
        Vector<u32> v;
        v.pushBack(10);
        v.pushBack(20);
        v.mut(0) = 100;
        v.mut(1) = 200;
        STD_INSIST(v[0] == 100);
        STD_INSIST(v[1] == 200);
    }

    STD_TEST(MutBack) {
        Vector<u32> v;
        v.pushBack(10);
        v.pushBack(20);
        v.mutBack() = 999;
        STD_INSIST(v.back() == 999);
        STD_INSIST(v[1] == 999);
    }

    STD_TEST(AppendArray) {
        Vector<u32> v;
        u32 arr[] = {1, 2, 3, 4, 5};
        v.append(arr, arr + 5);
        STD_INSIST(v.length() == 5);
        for (u32 i = 0; i < 5; ++i) {
            STD_INSIST(v[i] == i + 1);
        }
    }

    STD_TEST(AppendWithLength) {
        Vector<u32> v;
        u32 arr[] = {10, 20, 30};
        v.append(arr, 3);
        STD_INSIST(v.length() == 3);
        STD_INSIST(v[0] == 10);
        STD_INSIST(v[1] == 20);
        STD_INSIST(v[2] == 30);
    }

    STD_TEST(AppendToExisting) {
        Vector<u32> v;
        v.pushBack(1);
        u32 arr[] = {2, 3};
        v.append(arr, 2);
        STD_INSIST(v.length() == 3);
        STD_INSIST(v[0] == 1);
        STD_INSIST(v[1] == 2);
        STD_INSIST(v[2] == 3);
    }

    STD_TEST(AppendEmpty) {
        Vector<u32> v;
        v.pushBack(42);
        u32 arr[] = {};
        v.append(arr, (size_t)0);
        STD_INSIST(v.length() == 1);
        STD_INSIST(v[0] == 42);
    }

    STD_TEST(Iterators) {
        Vector<u32> v;
        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        u32 sum = 0;
        for (auto it = v.begin(); it != v.end(); ++it) {
            sum += *it;
        }
        STD_INSIST(sum == 6);
    }

    STD_TEST(MutIterators) {
        Vector<u32> v;
        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        for (auto it = v.mutBegin(); it != v.mutEnd(); ++it) {
            *it *= 2;
        }
        STD_INSIST(v[0] == 2);
        STD_INSIST(v[1] == 4);
        STD_INSIST(v[2] == 6);
    }

    STD_TEST(DataPointer) {
        Vector<u32> v;
        v.pushBack(10);
        v.pushBack(20);
        const u32* ptr = v.data();
        STD_INSIST(ptr[0] == 10);
        STD_INSIST(ptr[1] == 20);
    }

    STD_TEST(MutDataPointer) {
        Vector<u32> v;
        v.pushBack(10);
        v.pushBack(20);
        u32* ptr = v.mutData();
        ptr[0] = 100;
        ptr[1] = 200;
        STD_INSIST(v[0] == 100);
        STD_INSIST(v[1] == 200);
    }

    STD_TEST(MoveConstruction) {
        Vector<u32> v1;
        v1.pushBack(1);
        v1.pushBack(2);
        Vector<u32> v2(move(v1));
        STD_INSIST(v2.length() == 2);
        STD_INSIST(v2[0] == 1);
        STD_INSIST(v2[1] == 2);
    }

    STD_TEST(CopyConstruction) {
        Vector<u32> v1;
        v1.pushBack(1);
        v1.pushBack(2);
        Vector<u32> v2(v1);
        STD_INSIST(v2.length() == 2);
        STD_INSIST(v2[0] == 1);
        STD_INSIST(v2[1] == 2);
        STD_INSIST(v1.length() == 2);
    }

    STD_TEST(Xchg) {
        Vector<u32> v1;
        v1.pushBack(1);
        v1.pushBack(2);
        Vector<u32> v2;
        v2.pushBack(10);
        v2.pushBack(20);
        v2.pushBack(30);
        v1.xchg(v2);
        STD_INSIST(v1.length() == 3);
        STD_INSIST(v1[0] == 10);
        STD_INSIST(v1[1] == 20);
        STD_INSIST(v1[2] == 30);
        STD_INSIST(v2.length() == 2);
        STD_INSIST(v2[0] == 1);
        STD_INSIST(v2[1] == 2);
    }

    STD_TEST(XchgWithEmpty) {
        Vector<u32> v1;
        v1.pushBack(1);
        v1.pushBack(2);
        Vector<u32> v2;
        v1.xchg(v2);
        STD_INSIST(v1.empty());
        STD_INSIST(v2.length() == 2);
        STD_INSIST(v2[0] == 1);
        STD_INSIST(v2[1] == 2);
    }

    STD_TEST(Grow) {
        Vector<u32> v;
        v.grow(10);
        STD_INSIST(v.left() >= 10);
    }

    STD_TEST(GrowDelta) {
        Vector<u32> v;
        v.pushBack(1);
        v.pushBack(2);
        auto oldLen = v.length();
        v.growDelta(5);
        STD_INSIST(v.left() >= 5);
        STD_INSIST(v.length() == oldLen);
    }

    STD_TEST(StorageEnd) {
        Vector<u32> v;
        v.pushBack(1);
        STD_INSIST(v.storageEnd() >= v.end());
        STD_INSIST(v.left() == (size_t)(v.storageEnd() - v.end()));
    }

    STD_TEST(LeftCapacity) {
        Vector<u32> v;
        v.grow(10);
        v.pushBack(1);
        v.pushBack(2);
        auto left = v.left();
        STD_INSIST(left >= 8);
    }

    STD_TEST(MultiplePopBack) {
        Vector<u32> v;
        for (u32 i = 0; i < 10; ++i) {
            v.pushBack(i);
        }
        for (u32 i = 0; i < 5; ++i) {
            v.popBack();
        }
        STD_INSIST(v.length() == 5);
        for (u32 i = 0; i < 5; ++i) {
            STD_INSIST(v[i] == i);
        }
    }

    STD_TEST(LargeVector) {
        Vector<u32> v;
        for (u32 i = 0; i < 1000; ++i) {
            v.pushBack(i);
        }
        STD_INSIST(v.length() == 1000);
        for (u32 i = 0; i < 1000; ++i) {
            STD_INSIST(v[i] == i);
        }
    }

    STD_TEST(AppendLargeArray) {
        Vector<u32> v;
        u32 arr[100];
        for (u32 i = 0; i < 100; ++i) {
            arr[i] = i * 2;
        }
        v.append(arr, 100);
        STD_INSIST(v.length() == 100);
        for (u32 i = 0; i < 100; ++i) {
            STD_INSIST(v[i] == i * 2);
        }
    }

    STD_TEST(DifferentTypes) {
        Vector<u8> v;
        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        STD_INSIST(v.length() == 3);
        STD_INSIST(v[0] == 1);
        STD_INSIST(v[1] == 2);
        STD_INSIST(v[2] == 3);
    }
}
