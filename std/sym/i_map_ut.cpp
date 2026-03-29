#include "i_map.h"

#include <std/mem/obj_pool.h>

#include <std/tst/ut.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(IntMap) {
    STD_TEST(Simple) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> m(pool.mutPtr());

        m[0] = 0;
        m[1] = 1;

        STD_INSIST(m[0] == 0);
        STD_INSIST(m[1] == 1);
    }

    STD_TEST(DefaultConstructor) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        STD_INSIST(map.find(42) == nullptr);
    }

    STD_TEST(FindNonExistent) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        int* result = map.find(100);

        STD_INSIST(result == nullptr);
    }

    STD_TEST(FindExisting) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(42, 100);

        int* result = map.find(42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 100);
    }

    STD_TEST(FindZeroKey) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(0, 999);

        int* result = map.find(0);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 999);
    }

    STD_TEST(FindAfterMultipleInserts) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map.insert(1, 10);
        map.insert(2, 20);
        map.insert(3, 30);

        STD_INSIST(*map.find(1) == 10);
        STD_INSIST(*map.find(2) == 20);
        STD_INSIST(*map.find(3) == 30);
    }

    STD_TEST(FindConstCorrectness) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(42, 100);

        const IntMap<int>& constMap = map;
        int* result = constMap.find(42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 100);
    }

    STD_TEST(InsertBasic) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        int* result = map.insert(10, 42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
    }

    STD_TEST(InsertReturnsPointerToValue) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        int* inserted = map.insert(5, 123);
        int* found = map.find(5);

        STD_INSIST(inserted == found);
    }

    STD_TEST(InsertDefaultConstructed) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        int* result = map.insert(7);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 0);
    }

    STD_TEST(InsertWithMultipleArgs) {
        auto pool = ObjPool::fromMemory();
        struct Point {
            int x, y;
            Point(int x_, int y_)
                : x(x_)
                , y(y_)
            {
            }
        };

        IntMap<Point> map(pool.mutPtr());

        Point* result = map.insert(100, 10, 20);

        STD_INSIST(result != nullptr);
        STD_INSIST(result->x == 10);
        STD_INSIST(result->y == 20);
    }

    STD_TEST(InsertOverwritesExisting) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(50, 1);

        map.insert(50, 2);

        int* result = map.find(50);
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 2);
    }

    STD_TEST(InsertLargeKeys) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map.insert(1000000, 42);
        map.insert(999999999, 43);

        STD_INSIST(*map.find(1000000) == 42);
        STD_INSIST(*map.find(999999999) == 43);
    }

    STD_TEST(InsertManyKeys) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

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
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map[100];

        STD_INSIST(map.find(100) != nullptr);
    }

    STD_TEST(OperatorBracketReturnsExisting) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(25, 42);

        int& value = map[25];

        STD_INSIST(value == 42);
    }

    STD_TEST(OperatorBracketModifiable) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(30, 10);

        map[30] = 20;

        STD_INSIST(*map.find(30) == 20);
    }

    STD_TEST(OperatorBracketSamePointer) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(15, 42);

        int* found = map.find(15);
        int& ref = map[15];

        STD_INSIST(&ref == found);
    }

    STD_TEST(OperatorBracketNewValueModifiable) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map[99] = 100;

        STD_INSIST(*map.find(99) == 100);
    }

    STD_TEST(OperatorBracketChained) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map[1] = 10;
        map[2] = 20;
        map[3] = map[1] + map[2];

        STD_INSIST(map[3] == 30);
    }

    STD_TEST(StructValue) {
        auto pool = ObjPool::fromMemory();
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

        IntMap<Data> map(pool.mutPtr());
        map.insert(42, 100, 3.14f);

        Data* result = map.find(42);
        STD_INSIST(result != nullptr);
        STD_INSIST(result->id == 100);
        STD_INSIST(result->value == 3.14f);
    }

    STD_TEST(PointerValue) {
        auto pool = ObjPool::fromMemory();
        int x = 42;
        IntMap<int*> map(pool.mutPtr());

        map.insert(1, &x);

        int** result = map.find(1);
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == &x);
        STD_INSIST(**result == 42);
    }

    STD_TEST(NegativeKeys) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map.insert((u64)-1, 10);
        map.insert((u64)-100, 20);
        map.insert((u64)-999, 30);

        STD_INSIST(*map.find((u64)-1) == 10);
        STD_INSIST(*map.find((u64)-100) == 20);
        STD_INSIST(*map.find((u64)-999) == 30);
    }

    STD_TEST(ModifyThroughFind) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map.insert(10, 100);

        int* ptr = map.find(10);
        *ptr = 200;

        STD_INSIST(*map.find(10) == 200);
    }

    STD_TEST(MultipleMapInstances) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map1(pool.mutPtr());
        IntMap<int> map2(pool.mutPtr());

        map1.insert(1, 10);
        map2.insert(1, 20);

        STD_INSIST(*map1.find(1) == 10);
        STD_INSIST(*map2.find(1) == 20);
    }

    STD_TEST(InsertWithMove) {
        auto pool = ObjPool::fromMemory();
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

        IntMap<MoveOnly> map(pool.mutPtr());

        MoveOnly* result = map.insert(42, 100);

        STD_INSIST(result != nullptr);
        STD_INSIST(result->value == 100);
    }

    STD_TEST(KeyCollisions) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

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
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map.insert(0xFFFFFFFFFFFFFFFFULL, 100);
        map.insert(0x8000000000000000ULL, 200);
        map.insert(0x0000000000000001ULL, 300);

        STD_INSIST(*map.find(0xFFFFFFFFFFFFFFFFULL) == 100);
        STD_INSIST(*map.find(0x8000000000000000ULL) == 200);
        STD_INSIST(*map.find(0x0000000000000001ULL) == 300);
    }

    STD_TEST(SequentialKeys) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        for (u64 i = 0; i < 50; ++i) {
            map[i] = (int)(i + 1);
        }

        for (u64 i = 0; i < 50; ++i) {
            STD_INSIST(map[i] == (int)(i + 1));
        }
    }

    STD_TEST(SparseKeys) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

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
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map[42] = 1;
        map[42] = 2;
        map[42] = 3;

        STD_INSIST(*map.find(42) == 3);
    }

    STD_TEST(DifferentTypesInt) {
        auto pool = ObjPool::fromMemory();
        IntMap<u8> map(pool.mutPtr());

        map[1] = 255;
        map[2] = 128;

        STD_INSIST(*map.find(1) == 255);
        STD_INSIST(*map.find(2) == 128);
    }

    STD_TEST(DifferentTypesFloat) {
        auto pool = ObjPool::fromMemory();
        IntMap<float> map(pool.mutPtr());

        map[1] = 3.14f;
        map[2] = 2.71f;

        STD_INSIST(*map.find(1) == 3.14f);
        STD_INSIST(*map.find(2) == 2.71f);
    }

    STD_TEST(EraseNonExistent) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        map.erase(42);

        STD_INSIST(map.find(42) == nullptr);
    }

    STD_TEST(EraseExisting) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[10] = 100;

        map.erase(10);

        STD_INSIST(map.find(10) == nullptr);
    }

    STD_TEST(EraseAndReinsert) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[5] = 50;

        map.erase(5);
        map[5] = 100;

        STD_INSIST(*map.find(5) == 100);
    }

    STD_TEST(EraseMultiple) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[1] = 10;
        map[2] = 20;
        map[3] = 30;

        map.erase(2);

        STD_INSIST(*map.find(1) == 10);
        STD_INSIST(map.find(2) == nullptr);
        STD_INSIST(*map.find(3) == 30);
    }

    STD_TEST(EraseAllElements) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[1] = 10;
        map[2] = 20;
        map[3] = 30;

        map.erase(1);
        map.erase(2);
        map.erase(3);

        STD_INSIST(map.find(1) == nullptr);
        STD_INSIST(map.find(2) == nullptr);
        STD_INSIST(map.find(3) == nullptr);
    }

    STD_TEST(EraseZeroKey) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[0] = 999;
        map[1] = 111;

        map.erase(0);

        STD_INSIST(map.find(0) == nullptr);
        STD_INSIST(*map.find(1) == 111);
    }

    STD_TEST(EraseLargeKey) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[0xFFFFFFFFFFFFFFFFULL] = 100;
        map[1] = 10;

        map.erase(0xFFFFFFFFFFFFFFFFULL);

        STD_INSIST(map.find(0xFFFFFFFFFFFFFFFFULL) == nullptr);
        STD_INSIST(*map.find(1) == 10);
    }

    STD_TEST(EraseComplexType) {
        auto pool = ObjPool::fromMemory();
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

        IntMap<Data> map(pool.mutPtr());
        map.insert(1, 10, 1.5f);
        map.insert(2, 20, 2.5f);

        map.erase(1);

        STD_INSIST(map.find(1) == nullptr);
        Data* d2 = map.find(2);
        STD_INSIST(d2 != nullptr);
        STD_INSIST(d2->id == 20);
        STD_INSIST(d2->value == 2.5f);
    }

    STD_TEST(EraseWithVector) {
        auto pool = ObjPool::fromMemory();
        IntMap<Vector<int>> map(pool.mutPtr());
        map[1].pushBack(10);
        map[1].pushBack(20);
        map[2].pushBack(30);

        map.erase(1);

        STD_INSIST(map.find(1) == nullptr);
        STD_INSIST(map[2][0] == 30);
    }

    STD_TEST(EraseManyElements) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        for (int i = 0; i < 100; ++i) {
            map[i] = i * 10;
        }

        for (int i = 0; i < 50; ++i) {
            map.erase(i);
        }

        for (int i = 0; i < 50; ++i) {
            STD_INSIST(map.find(i) == nullptr);
        }

        for (int i = 50; i < 100; ++i) {
            STD_INSIST(*map.find(i) == i * 10);
        }
    }

    STD_TEST(EraseAlternatingElements) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        for (int i = 0; i < 20; ++i) {
            map[i] = i;
        }

        for (int i = 0; i < 20; i += 2) {
            map.erase(i);
        }

        for (int i = 0; i < 20; ++i) {
            if (i % 2 == 0) {
                STD_INSIST(map.find(i) == nullptr);
            } else {
                STD_INSIST(*map.find(i) == i);
            }
        }
    }

    STD_TEST(EraseSameKeyTwice) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[10] = 100;

        map.erase(10);
        map.erase(10);

        STD_INSIST(map.find(10) == nullptr);
    }

    STD_TEST(EraseCallsDestructor) {
        auto pool = ObjPool::fromMemory();
        struct TrackedObject {
            int* destructorCount;

            TrackedObject(int* counter)
                : destructorCount(counter)
            {
            }

            ~TrackedObject() {
                if (destructorCount) {
                    (*destructorCount)++;
                }
            }
        };

        int destructorCount = 0;
        IntMap<TrackedObject> map(pool.mutPtr());
        map.insert(1, &destructorCount);
        map.insert(2, &destructorCount);
        map.insert(3, &destructorCount);

        STD_INSIST(destructorCount == 0);

        map.erase(2);

        STD_INSIST(destructorCount == 1);
    }

    STD_TEST(EraseCallsDestructorMultipleTimes) {
        auto pool = ObjPool::fromMemory();
        struct TrackedObject {
            int* destructorCount;

            TrackedObject(int* counter)
                : destructorCount(counter)
            {
            }

            ~TrackedObject() {
                if (destructorCount) {
                    (*destructorCount)++;
                }
            }
        };

        int destructorCount = 0;

        {
            IntMap<TrackedObject> map(pool.mutPtr());

            map.insert(10, &destructorCount);
            map.insert(20, &destructorCount);
            map.insert(30, &destructorCount);
            map.insert(40, &destructorCount);
            map.insert(50, &destructorCount);

            STD_INSIST(destructorCount == 0);

            map.erase(10);
            STD_INSIST(destructorCount == 1);

            map.erase(30);
            STD_INSIST(destructorCount == 2);

            map.erase(50);
            STD_INSIST(destructorCount == 3);
        }

        STD_INSIST(destructorCount == 5);
    }

    STD_TEST(VisitEmpty) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        int count = 0;
        map.visit([&count](int&) {
            count++;
        });

        STD_INSIST(count == 0);
    }

    STD_TEST(VisitSingleElement) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[10] = 100;

        int count = 0;
        int sum = 0;
        map.visit([&count, &sum](int& value) {
            count++;
            sum += value;
        });

        STD_INSIST(count == 1);
        STD_INSIST(sum == 100);
    }

    STD_TEST(VisitMultipleElements) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[1] = 10;
        map[2] = 20;
        map[3] = 30;

        int count = 0;
        int sum = 0;
        map.visit([&count, &sum](int& value) {
            count++;
            sum += value;
        });

        STD_INSIST(count == 3);
        STD_INSIST(sum == 60);
    }

    STD_TEST(VisitModifyValues) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[1] = 10;
        map[2] = 20;
        map[3] = 30;

        map.visit([](int& value) {
            value *= 2;
        });

        STD_INSIST(*map.find(1) == 20);
        STD_INSIST(*map.find(2) == 40);
        STD_INSIST(*map.find(3) == 60);
    }

    STD_TEST(VisitComplexType) {
        auto pool = ObjPool::fromMemory();
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

        IntMap<Data> map(pool.mutPtr());
        map.insert(1, 10, 1.5f);
        map.insert(2, 20, 2.5f);

        int count = 0;
        float sumValues = 0.0f;
        map.visit([&count, &sumValues](Data& d) {
            count++;
            sumValues += d.value;
        });

        STD_INSIST(count == 2);
        STD_INSIST(sumValues == 4.0f);
    }

    STD_TEST(VisitModifyComplexType) {
        auto pool = ObjPool::fromMemory();
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

        IntMap<Data> map(pool.mutPtr());
        map.insert(1, 10, 1.0f);
        map.insert(2, 20, 2.0f);

        map.visit([](Data& d) {
            d.id *= 10;
            d.value *= 10.0f;
        });

        Data* d1 = map.find(1);
        Data* d2 = map.find(2);

        STD_INSIST(d1->id == 100);
        STD_INSIST(d1->value == 10.0f);
        STD_INSIST(d2->id == 200);
        STD_INSIST(d2->value == 20.0f);
    }

    STD_TEST(VisitWithVector) {
        auto pool = ObjPool::fromMemory();
        IntMap<Vector<int>> map(pool.mutPtr());
        map[1].pushBack(10);
        map[1].pushBack(20);
        map[2].pushBack(30);
        map[2].pushBack(40);
        map[2].pushBack(50);

        int totalElements = 0;
        map.visit([&totalElements](Vector<int>& vec) {
            totalElements += (int)vec.length();
        });

        STD_INSIST(totalElements == 5);
    }

    STD_TEST(VisitManyElements) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());

        for (int i = 0; i < 100; ++i) {
            map[i] = i * 2;
        }

        int count = 0;
        int sum = 0;
        map.visit([&count, &sum](int& value) {
            count++;
            sum += value;
        });

        STD_INSIST(count == 100);
        STD_INSIST(sum == 9900);
    }

    STD_TEST(VisitAfterErase) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[1] = 10;
        map[2] = 20;
        map[3] = 30;

        map.erase(2);

        int count = 0;
        int sum = 0;
        map.visit([&count, &sum](int& value) {
            count++;
            sum += value;
        });

        STD_INSIST(count == 2);
        STD_INSIST(sum == 40);
    }

    STD_TEST(VisitZeroKey) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[0] = 999;
        map[1] = 111;

        int count = 0;
        int sum = 0;
        map.visit([&count, &sum](int& value) {
            count++;
            sum += value;
        });

        STD_INSIST(count == 2);
        STD_INSIST(sum == 1110);
    }

    STD_TEST(VisitReadOnly) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[1] = 10;
        map[2] = 20;
        map[3] = 30;

        int maxValue = 0;
        map.visit([&maxValue](int& value) {
            if (value > maxValue) {
                maxValue = value;
            }
        });

        STD_INSIST(maxValue == 30);
    }

    STD_TEST(VisitCollectKeys) {
        auto pool = ObjPool::fromMemory();
        IntMap<int> map(pool.mutPtr());
        map[10] = 100;
        map[20] = 200;
        map[30] = 300;

        Vector<int> values;
        map.visit([&values](int& value) {
            values.pushBack(value);
        });

        STD_INSIST(values.length() == 3);

        bool found100 = false;
        bool found200 = false;
        bool found300 = false;

        for (size_t i = 0; i < values.length(); ++i) {
            if (values[i] == 100) {
                found100 = true;
            }
            if (values[i] == 200) {
                found200 = true;
            }
            if (values[i] == 300) {
                found300 = true;
            }
        }

        STD_INSIST(found100);
        STD_INSIST(found200);
        STD_INSIST(found300);
    }
}
