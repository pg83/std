#include "i_map.h"

#include <std/tst/ut.h>
#include <std/lib/vector.h>

using namespace Std;

STD_TEST_SUITE(IntMap) {
    STD_TEST(Simple) {
        IntMap<int> m;

        m[0] = 0;
        m[1] = 1;

        STD_INSIST(m[0] == 0);
        STD_INSIST(m[1] == 1);
    }

    STD_TEST(DefaultConstructor) {
        IntMap<int> map;

        STD_INSIST(map.find(42) == nullptr);
    }

    STD_TEST(FindNonExistent) {
        IntMap<int> map;

        int* result = map.find(100);

        STD_INSIST(result == nullptr);
    }

    STD_TEST(FindExisting) {
        IntMap<int> map;
        map.insert(42, 100);

        int* result = map.find(42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 100);
    }

    STD_TEST(FindZeroKey) {
        IntMap<int> map;
        map.insert(0, 999);

        int* result = map.find(0);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 999);
    }

    STD_TEST(FindAfterMultipleInserts) {
        IntMap<int> map;

        map.insert(1, 10);
        map.insert(2, 20);
        map.insert(3, 30);

        STD_INSIST(*map.find(1) == 10);
        STD_INSIST(*map.find(2) == 20);
        STD_INSIST(*map.find(3) == 30);
    }

    STD_TEST(FindConstCorrectness) {
        IntMap<int> map;
        map.insert(42, 100);

        const IntMap<int>& constMap = map;
        int* result = constMap.find(42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 100);
    }

    STD_TEST(InsertBasic) {
        IntMap<int> map;

        int* result = map.insert(10, 42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
    }

    STD_TEST(InsertReturnsPointerToValue) {
        IntMap<int> map;

        int* inserted = map.insert(5, 123);
        int* found = map.find(5);

        STD_INSIST(inserted == found);
    }

    STD_TEST(InsertDefaultConstructed) {
        IntMap<int> map;

        int* result = map.insert(7);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 0);
    }

    STD_TEST(InsertWithMultipleArgs) {
        struct Point {
            int x, y;
            Point(int x_, int y_)
                : x(x_)
                , y(y_)
            {
            }
        };

        IntMap<Point> map;

        Point* result = map.insert(100, 10, 20);

        STD_INSIST(result != nullptr);
        STD_INSIST(result->x == 10);
        STD_INSIST(result->y == 20);
    }

    STD_TEST(InsertOverwritesExisting) {
        IntMap<int> map;
        map.insert(50, 1);

        map.insert(50, 2);

        int* result = map.find(50);
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 2);
    }

    STD_TEST(InsertLargeKeys) {
        IntMap<int> map;

        map.insert(1000000, 42);
        map.insert(999999999, 43);

        STD_INSIST(*map.find(1000000) == 42);
        STD_INSIST(*map.find(999999999) == 43);
    }

    STD_TEST(InsertManyKeys) {
        IntMap<int> map;

        for (int i = 0; i < 100; ++i) {
            map.insert(i, i * 10);
        }

        for (int i = 0; i < 100; ++i) {
            int* result = map.find(i);
            STD_INSIST(result != nullptr);
            STD_INSIST(*result == i * 10);
        }
    }

    STD_TEST(OperatorBracketCreatesNew) {
        IntMap<int> map;

        int& value = map[100];

        STD_INSIST(map.find(100) != nullptr);
    }

    STD_TEST(OperatorBracketReturnsExisting) {
        IntMap<int> map;
        map.insert(25, 42);

        int& value = map[25];

        STD_INSIST(value == 42);
    }

    STD_TEST(OperatorBracketModifiable) {
        IntMap<int> map;
        map.insert(30, 10);

        map[30] = 20;

        STD_INSIST(*map.find(30) == 20);
    }

    STD_TEST(OperatorBracketSamePointer) {
        IntMap<int> map;
        map.insert(15, 42);

        int* found = map.find(15);
        int& ref = map[15];

        STD_INSIST(&ref == found);
    }

    STD_TEST(OperatorBracketNewValueModifiable) {
        IntMap<int> map;

        map[99] = 100;

        STD_INSIST(*map.find(99) == 100);
    }

    STD_TEST(OperatorBracketChained) {
        IntMap<int> map;

        map[1] = 10;
        map[2] = 20;
        map[3] = map[1] + map[2];

        STD_INSIST(map[3] == 30);
    }

    STD_TEST(StructValue) {
        struct Data {
            int id;
            float value;

            Data()
                : id(0)
                , value(0.0f)
            {
            }
            Data(int i, float v)
                : id(i)
                , value(v)
            {
            }
        };

        IntMap<Data> map;
        map.insert(42, 100, 3.14f);

        Data* result = map.find(42);
        STD_INSIST(result != nullptr);
        STD_INSIST(result->id == 100);
        STD_INSIST(result->value == 3.14f);
    }

    STD_TEST(PointerValue) {
        int x = 42;
        IntMap<int*> map;

        map.insert(1, &x);

        int** result = map.find(1);
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == &x);
        STD_INSIST(**result == 42);
    }

    STD_TEST(NegativeKeys) {
        IntMap<int> map;

        map.insert((u64)-1, 10);
        map.insert((u64)-100, 20);
        map.insert((u64)-999, 30);

        STD_INSIST(*map.find((u64)-1) == 10);
        STD_INSIST(*map.find((u64)-100) == 20);
        STD_INSIST(*map.find((u64)-999) == 30);
    }

    STD_TEST(ModifyThroughFind) {
        IntMap<int> map;
        map.insert(10, 100);

        int* ptr = map.find(10);
        *ptr = 200;

        STD_INSIST(*map.find(10) == 200);
    }

    STD_TEST(MultipleMapInstances) {
        IntMap<int> map1;
        IntMap<int> map2;

        map1.insert(1, 10);
        map2.insert(1, 20);

        STD_INSIST(*map1.find(1) == 10);
        STD_INSIST(*map2.find(1) == 20);
    }

    STD_TEST(InsertWithMove) {
        struct MoveOnly {
            int value;
            bool moved = false;

            MoveOnly(int v)
                : value(v)
            {
            }

            MoveOnly(MoveOnly&& other)
                : value(other.value)
                , moved(true)
            {
                other.value = 0;
            }
            MoveOnly(const MoveOnly&) = delete;
        };

        IntMap<MoveOnly> map;

        MoveOnly* result = map.insert(42, 100);

        STD_INSIST(result != nullptr);
        STD_INSIST(result->value == 100);
    }

    STD_TEST(Compactify) {
        IntMap<int> m;

        m[10] = 100;
        m[20] = 200;

        m.compactify();

        STD_INSIST(m[10] == 100);
        STD_INSIST(m[20] == 200);
    }

    STD_TEST(CompactifyEmpty) {
        IntMap<int> m;

        m.compactify();

        STD_INSIST(m.find(42) == nullptr);
    }

    STD_TEST(CompactifyManyElements) {
        IntMap<int> m;

        for (int i = 0; i < 100; ++i) {
            m[i] = i * 10;
        }

        m.compactify();

        for (int i = 0; i < 100; ++i) {
            STD_INSIST(m[i] == i * 10);
        }
    }

    STD_TEST(CompactifyPreservesValues) {
        IntMap<int> m;

        m[1] = 100;
        m[2] = 200;
        m[3] = 300;

        m.compactify();

        STD_INSIST(*m.find(1) == 100);
        STD_INSIST(*m.find(2) == 200);
        STD_INSIST(*m.find(3) == 300);
    }

    STD_TEST(CompactifyComplexType) {
        struct Data {
            int id;
            float value;

            Data()
                : id(0)
                , value(0.0f)
            {
            }
            Data(int i, float v)
                : id(i)
                , value(v)
            {
            }
        };

        IntMap<Data> m;

        m.insert(1, 10, 1.5f);
        m.insert(2, 20, 2.5f);
        m.insert(3, 30, 3.5f);

        m.compactify();

        Data* d1 = m.find(1);
        Data* d2 = m.find(2);
        Data* d3 = m.find(3);

        STD_INSIST(d1 != nullptr);
        STD_INSIST(d1->id == 10);
        STD_INSIST(d1->value == 1.5f);

        STD_INSIST(d2 != nullptr);
        STD_INSIST(d2->id == 20);
        STD_INSIST(d2->value == 2.5f);

        STD_INSIST(d3 != nullptr);
        STD_INSIST(d3->id == 30);
        STD_INSIST(d3->value == 3.5f);
    }

    STD_TEST(CompactifyChangesPointers) {
        IntMap<int> m;

        m[42] = 100;

        int* oldPtr = m.find(42);

        m.compactify();

        int* newPtr = m.find(42);

        STD_INSIST(oldPtr != newPtr);
        STD_INSIST(*newPtr == 100);
    }

    STD_TEST(CompactifyMultipleTimes) {
        IntMap<int> m;

        m[1] = 10;
        m[2] = 20;

        m.compactify();

        STD_INSIST(m[1] == 10);
        STD_INSIST(m[2] == 20);

        m.compactify();

        STD_INSIST(m[1] == 10);
        STD_INSIST(m[2] == 20);

        m.compactify();

        STD_INSIST(m[1] == 10);
        STD_INSIST(m[2] == 20);
    }

    STD_TEST(CompactifyThenInsert) {
        IntMap<int> m;

        m[100] = 10;

        m.compactify();

        m[200] = 20;

        STD_INSIST(m[100] == 10);
        STD_INSIST(m[200] == 20);
    }

    STD_TEST(CompactifyThenModify) {
        IntMap<int> m;

        m[50] = 100;

        m.compactify();

        m[50] = 200;

        STD_INSIST(*m.find(50) == 200);
    }

    STD_TEST(CompactifyWithVectorValues) {
        IntMap<Vector<int>> m;

        m[1].pushBack(10);
        m[1].pushBack(20);

        m[2].pushBack(30);
        m[2].pushBack(40);
        m[2].pushBack(50);

        STD_INSIST(m[1].length() == 2);
        STD_INSIST(m[1][0] == 10);
        STD_INSIST(m[1][1] == 20);

        STD_INSIST(m[2].length() == 3);
        STD_INSIST(m[2][0] == 30);
        STD_INSIST(m[2][1] == 40);
        STD_INSIST(m[2][2] == 50);

        m.compactify();

        STD_INSIST(m.find(1) != nullptr);
        STD_INSIST(m.find(2) != nullptr);

        STD_INSIST(m[1].length() == 2);
        STD_INSIST(m[1][0] == 10);
        STD_INSIST(m[1][1] == 20);

        STD_INSIST(m[2].length() == 3);
        STD_INSIST(m[2][0] == 30);
        STD_INSIST(m[2][1] == 40);
        STD_INSIST(m[2][2] == 50);
    }

    STD_TEST(CompactifyWithZeroKey) {
        IntMap<int> m;

        m[0] = 999;
        m[1] = 111;

        m.compactify();

        STD_INSIST(*m.find(0) == 999);
        STD_INSIST(*m.find(1) == 111);
    }

    STD_TEST(CompactifyPreservesAllKeys) {
        IntMap<int> m;

        m[10] = 1;
        m[20] = 2;
        m[30] = 3;
        m[40] = 4;
        m[50] = 5;

        m.compactify();

        STD_INSIST(m.find(10) != nullptr);
        STD_INSIST(m.find(20) != nullptr);
        STD_INSIST(m.find(30) != nullptr);
        STD_INSIST(m.find(40) != nullptr);
        STD_INSIST(m.find(50) != nullptr);

        STD_INSIST(*m.find(10) == 1);
        STD_INSIST(*m.find(20) == 2);
        STD_INSIST(*m.find(30) == 3);
        STD_INSIST(*m.find(40) == 4);
        STD_INSIST(*m.find(50) == 5);
    }

    STD_TEST(CompactifySingleElement) {
        IntMap<int> m;

        m[999] = 42;

        m.compactify();

        STD_INSIST(*m.find(999) == 42);
    }

    STD_TEST(CompactifyLargeValues) {
        struct LargeStruct {
            int data[100];

            LargeStruct() {
                for (int i = 0; i < 100; ++i) {
                    data[i] = i;
                }
            }
        };

        IntMap<LargeStruct> m;

        m.insert(1);
        m.insert(2);

        m.compactify();

        LargeStruct* ls1 = m.find(1);
        LargeStruct* ls2 = m.find(2);

        STD_INSIST(ls1 != nullptr);
        STD_INSIST(ls2 != nullptr);

        for (int i = 0; i < 100; ++i) {
            STD_INSIST(ls1->data[i] == i);
            STD_INSIST(ls2->data[i] == i);
        }
    }

    STD_TEST(KeyCollisions) {
        IntMap<int> map;

        for (u64 i = 0; i < 1000; ++i) {
            map.insert(i, (int)(i * 2));
        }

        for (u64 i = 0; i < 1000; ++i) {
            int* result = map.find(i);
            STD_INSIST(result != nullptr);
            STD_INSIST(*result == (int)(i * 2));
        }
    }

    STD_TEST(LargeKeyValues) {
        IntMap<int> map;

        map.insert(0xFFFFFFFFFFFFFFFFULL, 100);
        map.insert(0x8000000000000000ULL, 200);
        map.insert(0x0000000000000001ULL, 300);

        STD_INSIST(*map.find(0xFFFFFFFFFFFFFFFFULL) == 100);
        STD_INSIST(*map.find(0x8000000000000000ULL) == 200);
        STD_INSIST(*map.find(0x0000000000000001ULL) == 300);
    }

    STD_TEST(SequentialKeys) {
        IntMap<int> map;

        for (u64 i = 0; i < 50; ++i) {
            map[i] = (int)(i + 1);
        }

        for (u64 i = 0; i < 50; ++i) {
            STD_INSIST(map[i] == (int)(i + 1));
        }
    }

    STD_TEST(SparseKeys) {
        IntMap<int> map;

        map[0] = 1;
        map[1000] = 2;
        map[1000000] = 3;
        map[1000000000] = 4;

        STD_INSIST(map[0] == 1);
        STD_INSIST(map[1000] == 2);
        STD_INSIST(map[1000000] == 3);
        STD_INSIST(map[1000000000] == 4);
    }

    STD_TEST(OverwriteAndFind) {
        IntMap<int> map;

        map[42] = 1;
        map[42] = 2;
        map[42] = 3;

        STD_INSIST(*map.find(42) == 3);
    }

    STD_TEST(DifferentTypesInt) {
        IntMap<u8> map;

        map[1] = 255;
        map[2] = 128;

        STD_INSIST(*map.find(1) == 255);
        STD_INSIST(*map.find(2) == 128);
    }

    STD_TEST(DifferentTypesFloat) {
        IntMap<float> map;

        map[1] = 3.14f;
        map[2] = 2.71f;

        STD_INSIST(*map.find(1) == 3.14f);
        STD_INSIST(*map.find(2) == 2.71f);
    }
}
