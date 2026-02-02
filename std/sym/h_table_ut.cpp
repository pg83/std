#include "h_table.h"

#include <std/tst/ut.h>
#include <std/rng/pcg.h>

#include <string.h>

using namespace Std;

STD_TEST_SUITE(HashTable) {
    STD_TEST(basicSetAndFind) {
        HashTable ht;

        int value1 = 100;
        int value2 = 200;
        int value3 = 300;

        ht.set(1, &value1);
        ht.set(2, &value2);
        ht.set(3, &value3);

        STD_INSIST(ht.find(1) == &value1);
        STD_INSIST(ht.find(2) == &value2);
        STD_INSIST(ht.find(3) == &value3);
        STD_INSIST(ht.size() == 3);
    }

    STD_TEST(findNonExistent) {
        HashTable ht;

        int value = 42;
        ht.set(1, &value);

        STD_INSIST(ht.find(999) == nullptr);
        STD_INSIST(ht.find(0) == nullptr);
        STD_INSIST(ht.find(2) == nullptr);
    }

    STD_TEST(updateExisting) {
        HashTable ht;

        int value1 = 100;
        int value2 = 200;

        ht.set(1, &value1);
        STD_INSIST(ht.find(1) == &value1);
        STD_INSIST(ht.size() == 1);

        ht.set(1, &value2);
        STD_INSIST(ht.find(1) == &value2);
        STD_INSIST(ht.size() == 1);
    }

    STD_TEST(largeNumberOfElements) {
        HashTable ht;

        const size_t count = 10000;
        int* values = new int[count];

        for (size_t i = 0; i < count; ++i) {
            values[i] = i * 7;
        }

        for (size_t i = 0; i < count; ++i) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.size() == count);

        for (size_t i = 0; i < count; ++i) {
            int* found = static_cast<int*>(ht.find(i + 1));
            STD_INSIST(found != nullptr);
            STD_INSIST(*found == i * 7);
        }

        delete[] values;
    }

    STD_TEST(rehashWorks) {
        HashTable ht(4);

        int values[20];
        for (int i = 0; i < 20; ++i) {
            values[i] = i * 10;
        }

        size_t initialCapacity = ht.capacity();

        for (int i = 0; i < 20; ++i) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.capacity() > initialCapacity);
        STD_INSIST(ht.size() == 20);

        for (int i = 0; i < 20; ++i) {
            int* found = static_cast<int*>(ht.find(i + 1));
            STD_INSIST(found != nullptr);
            STD_INSIST(*found == i * 10);
        }
    }

    STD_TEST(collisionHandling) {
        HashTable ht(16);

        int values[5];
        for (int i = 0; i < 5; ++i) {
            values[i] = i;
        }

        ht.set(17, &values[0]);
        ht.set(33, &values[1]);
        ht.set(49, &values[2]);
        ht.set(65, &values[3]);
        ht.set(81, &values[4]);

        STD_INSIST(ht.size() == 5);

        STD_INSIST(ht.find(17) == &values[0]);
        STD_INSIST(ht.find(33) == &values[1]);
        STD_INSIST(ht.find(49) == &values[2]);
        STD_INSIST(ht.find(65) == &values[3]);
        STD_INSIST(ht.find(81) == &values[4]);
    }

    STD_TEST(sequentialKeys) {
        HashTable ht;

        const size_t count = 1000;
        int* values = new int[count];

        for (size_t i = 0; i < count; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            int* found = static_cast<int*>(ht.find(i + 1));
            STD_INSIST(found == &values[i]);
        }

        delete[] values;
    }

    STD_TEST(randomKeys) {
        HashTable ht;

        uint64_t keys[] = {
            12345678901234567ULL,
            98765432109876543ULL,
            11111111111111111ULL,
            99999999999999999ULL,
            55555555555555555ULL,
            77777777777777777ULL,
            33333333333333333ULL,
            88888888888888888ULL,
        };

        int values[8];
        for (int i = 0; i < 8; ++i) {
            values[i] = i * 100;
            ht.set(keys[i], &values[i]);
        }

        STD_INSIST(ht.size() == 8);

        for (int i = 0; i < 8; ++i) {
            int* found = static_cast<int*>(ht.find(keys[i]));
            STD_INSIST(found == &values[i]);
            STD_INSIST(*found == i * 100);
        }
    }

    STD_TEST(mixedOperations) {
        HashTable ht;

        int values[100];
        for (int i = 0; i < 100; ++i) {
            values[i] = i;
        }

        for (int i = 0; i < 100; i += 2) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.size() == 50);

        for (int i = 1; i < 100; i += 2) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.size() == 100);

        for (int i = 0; i < 100; i += 5) {
            ht.erase(i + 1);
        }

        STD_INSIST(ht.size() == 80);

        for (int i = 0; i < 100; ++i) {
            void* found = ht.find(i + 1);
            if (i % 5 == 0) {
                STD_INSIST(found == nullptr);
            } else {
                STD_INSIST(found == &values[i]);
            }
        }
    }

    STD_TEST(emptyTable) {
        HashTable ht;

        STD_INSIST(ht.size() == 0);
        STD_INSIST(ht.capacity() >= 8);
        STD_INSIST(ht.find(1) == nullptr);
        STD_INSIST(ht.find(999) == nullptr);
    }

    STD_TEST(singleElement) {
        HashTable ht;

        int value = 42;
        ht.set(123, &value);

        STD_INSIST(ht.size() == 1);
        STD_INSIST(ht.find(123) == &value);
        STD_INSIST(ht.find(124) == nullptr);
    }

    STD_TEST(pointerValues) {
        HashTable ht;

        int a = 1, b = 2, c = 3;
        int* ptrA = &a;
        int* ptrB = &b;
        int* ptrC = &c;

        ht.set(1, &ptrA);
        ht.set(2, &ptrB);
        ht.set(3, &ptrC);

        int** foundA = static_cast<int**>(ht.find(1));
        int** foundB = static_cast<int**>(ht.find(2));
        int** foundC = static_cast<int**>(ht.find(3));

        STD_INSIST(foundA != nullptr && *foundA == &a);
        STD_INSIST(foundB != nullptr && *foundB == &b);
        STD_INSIST(foundC != nullptr && *foundC == &c);
    }

    STD_TEST(structValues) {
        struct Data {
            int x;
            double y;
            const char* name;
        };

        HashTable ht;

        Data d1 = {10, 1.5, "first"};
        Data d2 = {20, 2.5, "second"};
        Data d3 = {30, 3.5, "third"};

        ht.set(100, &d1);
        ht.set(200, &d2);
        ht.set(300, &d3);

        Data* found1 = static_cast<Data*>(ht.find(100));
        Data* found2 = static_cast<Data*>(ht.find(200));
        Data* found3 = static_cast<Data*>(ht.find(300));

        STD_INSIST(found1->x == 10 && found1->y == 1.5);
        STD_INSIST(found2->x == 20 && found2->y == 2.5);
        STD_INSIST(found3->x == 30 && found3->y == 3.5);
        STD_INSIST(strcmp(found1->name, "first") == 0);
    }

    STD_TEST(forEachEmptyTable) {
        HashTable ht;

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        ht.forEach(it);

        STD_INSIST(it.count == 0);
    }

    STD_TEST(forEachSingleElement) {
        HashTable ht;
        int value = 42;
        ht.set(1, &value);

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void** lastEl = nullptr;
            void process(void** el) override {
                count++;
                lastEl = el;
            }
        };

        CountIterator it;
        ht.forEach(it);

        STD_INSIST(it.count == 1);
        STD_INSIST(it.lastEl != nullptr);
        STD_INSIST(*it.lastEl == &value);
    }

    STD_TEST(forEachMultipleElements) {
        HashTable ht;

        int values[5] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        ht.forEach(it);

        STD_INSIST(it.count == 5);
        STD_INSIST(ht.size() == 5);
    }

    STD_TEST(forEachVisitsAllElements) {
        HashTable ht;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i * 10;
            ht.set(i + 1, &values[i]);
        }

        struct SumIterator: HashTable::Iterator {
            int sum = 0;
            void process(void** el) override {
                int* ptr = static_cast<int*>(*el);
                sum += *ptr;
            }
        };

        SumIterator it;
        ht.forEach(it);

        STD_INSIST(it.sum == 0 + 10 + 20 + 30 + 40 + 50 + 60 + 70 + 80 + 90);
    }

    STD_TEST(forEachModifyValues) {
        HashTable ht;

        int values[5] = {1, 2, 3, 4, 5};
        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        struct MultiplyIterator: HashTable::Iterator {
            void process(void** el) override {
                int* ptr = static_cast<int*>(*el);
                *ptr *= 2;
            }
        };

        MultiplyIterator it;
        ht.forEach(it);

        for (int i = 0; i < 5; ++i) {
            int* found = static_cast<int*>(ht.find(i + 1));
            STD_INSIST(found != nullptr);
            STD_INSIST(*found == (i + 1) * 2);
        }
    }

    STD_TEST(forEachLargeTable) {
        HashTable ht;

        const size_t count = 1000;
        int* values = new int[count];

        for (size_t i = 0; i < count; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        struct CountIterator: HashTable::Iterator {
            size_t count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        ht.forEach(it);

        STD_INSIST(it.count == count);
        STD_INSIST(it.count == ht.size());

        delete[] values;
    }

    STD_TEST(forEachCollectPointers) {
        HashTable ht;

        int values[5] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        struct CollectIterator: HashTable::Iterator {
            void* collected[5];
            int index = 0;
            void process(void** el) override {
                if (index < 5) {
                    collected[index++] = *el;
                }
            }
        };

        CollectIterator it;
        ht.forEach(it);

        STD_INSIST(it.index == 5);

        bool found[5] = {false, false, false, false, false};
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) {
                if (it.collected[i] == &values[j]) {
                    found[j] = true;
                }
            }
        }

        for (int i = 0; i < 5; ++i) {
            STD_INSIST(found[i]);
        }
    }

    STD_TEST(forEachAfterRehash) {
        HashTable ht(4);

        int values[20];
        for (int i = 0; i < 20; ++i) {
            values[i] = i * 7;
            ht.set(i + 1, &values[i]);
        }

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        ht.forEach(it);

        STD_INSIST(it.count == 20);
        STD_INSIST(it.count == ht.size());
    }

    STD_TEST(forEachWithStructs) {
        struct Data {
            int x;
            const char* name;
        };

        HashTable ht;

        Data d1 = {10, "first"};
        Data d2 = {20, "second"};
        Data d3 = {30, "third"};

        ht.set(1, &d1);
        ht.set(2, &d2);
        ht.set(3, &d3);

        struct SumIterator: HashTable::Iterator {
            int sum = 0;
            void process(void** el) override {
                Data* data = static_cast<Data*>(*el);
                sum += data->x;
            }
        };

        SumIterator it;
        ht.forEach(it);

        STD_INSIST(it.sum == 60);
    }

    STD_TEST(eraseBasic) {
        HashTable ht;

        int value1 = 100;
        int value2 = 200;
        int value3 = 300;

        ht.set(1, &value1);
        ht.set(2, &value2);
        ht.set(3, &value3);

        STD_INSIST(ht.size() == 3);

        ht.erase(2);

        STD_INSIST(ht.find(1) == &value1);
        STD_INSIST(ht.find(2) == nullptr);
        STD_INSIST(ht.find(3) == &value3);

        STD_INSIST(ht.size() == 2);
    }

    STD_TEST(eraseNonExistent) {
        HashTable ht;

        int value = 42;
        ht.set(1, &value);

        STD_INSIST(ht.size() == 1);

        ht.erase(999);
        ht.erase(0);
        ht.erase(2);

        STD_INSIST(ht.size() == 1);
        STD_INSIST(ht.find(1) == &value);
    }

    STD_TEST(eraseAndReinsert) {
        HashTable ht;

        int value1 = 100;
        int value2 = 200;

        ht.set(1, &value1);
        STD_INSIST(ht.find(1) == &value1);

        ht.erase(1);
        STD_INSIST(ht.find(1) == nullptr);

        ht.set(1, &value2);
        STD_INSIST(ht.find(1) == &value2);
    }

    STD_TEST(eraseAllElements) {
        HashTable ht;

        int values[5] = {10, 20, 30, 40, 50};

        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.size() == 5);

        for (int i = 0; i < 5; ++i) {
            ht.erase(i + 1);
        }

        for (int i = 0; i < 5; ++i) {
            STD_INSIST(ht.find(i + 1) == nullptr);
        }

        STD_INSIST(ht.size() == 0);
    }

    STD_TEST(eraseFromEmpty) {
        HashTable ht;

        STD_INSIST(ht.size() == 0);

        ht.erase(1);
        ht.erase(999);

        STD_INSIST(ht.size() == 0);
    }

    STD_TEST(eraseAndFind) {
        HashTable ht;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i * 10;
            ht.set(i + 1, &values[i]);
        }

        ht.erase(3);
        ht.erase(7);
        ht.erase(9);

        for (int i = 0; i < 10; ++i) {
            int key = i + 1;
            void* found = ht.find(key);

            if (key == 3 || key == 7 || key == 9) {
                STD_INSIST(found == nullptr);
            } else {
                STD_INSIST(found == &values[i]);
            }
        }
    }

    STD_TEST(eraseWithCollisions) {
        HashTable ht(16);

        int values[5];
        for (int i = 0; i < 5; ++i) {
            values[i] = i;
        }

        ht.set(17, &values[0]);
        ht.set(33, &values[1]);
        ht.set(49, &values[2]);
        ht.set(65, &values[3]);
        ht.set(81, &values[4]);

        ht.erase(49);

        STD_INSIST(ht.find(17) == &values[0]);
        STD_INSIST(ht.find(33) == &values[1]);
        STD_INSIST(ht.find(49) == nullptr);
        STD_INSIST(ht.find(65) == &values[3]);
        STD_INSIST(ht.find(81) == &values[4]);
    }

    STD_TEST(eraseTwice) {
        HashTable ht;

        int value = 42;
        ht.set(1, &value);

        ht.erase(1);
        STD_INSIST(ht.find(1) == nullptr);

        ht.erase(1);
        STD_INSIST(ht.find(1) == nullptr);
    }

    STD_TEST(eraseAfterUpdate) {
        HashTable ht;

        int value1 = 100;
        int value2 = 200;

        ht.set(1, &value1);
        ht.set(1, &value2);

        STD_INSIST(ht.find(1) == &value2);

        ht.erase(1);

        STD_INSIST(ht.find(1) == nullptr);
    }

    STD_TEST(eraseMultipleTimes) {
        HashTable ht;

        int values[20];
        for (int i = 0; i < 20; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        for (int i = 0; i < 20; i += 2) {
            ht.erase(i + 1);
        }

        for (int i = 0; i < 20; ++i) {
            void* found = ht.find(i + 1);
            if (i % 2 == 0) {
                STD_INSIST(found == nullptr);
            } else {
                STD_INSIST(found == &values[i]);
            }
        }
    }

    STD_TEST(eraseAfterRehash) {
        HashTable ht(4);

        int values[20];
        for (int i = 0; i < 20; ++i) {
            values[i] = i * 10;
            ht.set(i + 1, &values[i]);
        }

        size_t sizeBefore = ht.size();
        STD_INSIST(sizeBefore == 20);

        ht.erase(5);
        ht.erase(10);
        ht.erase(15);

        STD_INSIST(ht.find(5) == nullptr);
        STD_INSIST(ht.find(10) == nullptr);
        STD_INSIST(ht.find(15) == nullptr);

        for (int i = 0; i < 20; ++i) {
            int key = i + 1;
            if (key != 5 && key != 10 && key != 15) {
                STD_INSIST(ht.find(key) == &values[i]);
            }
        }
    }

    STD_TEST(eraseAndForEach) {
        HashTable ht;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        ht.erase(3);
        ht.erase(5);
        ht.erase(7);

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        ht.forEach(it);

        STD_INSIST(it.count == 7);
    }

    STD_TEST(eraseWithLargeKeys) {
        HashTable ht;

        uint64_t keys[] = {
            12345678901234567ULL,
            98765432109876543ULL,
            11111111111111111ULL,
            99999999999999999ULL,
            55555555555555555ULL,
        };

        int values[5];
        for (int i = 0; i < 5; ++i) {
            values[i] = i * 100;
            ht.set(keys[i], &values[i]);
        }

        ht.erase(keys[2]);

        STD_INSIST(ht.find(keys[0]) == &values[0]);
        STD_INSIST(ht.find(keys[1]) == &values[1]);
        STD_INSIST(ht.find(keys[2]) == nullptr);
        STD_INSIST(ht.find(keys[3]) == &values[3]);
        STD_INSIST(ht.find(keys[4]) == &values[4]);
    }

    STD_TEST(eraseMixedWithInserts) {
        HashTable ht;

        int values[100];
        for (int i = 0; i < 100; ++i) {
            values[i] = i;
        }

        for (int i = 0; i < 50; ++i) {
            ht.set(i + 1, &values[i]);
        }

        for (int i = 0; i < 25; ++i) {
            ht.erase(i + 1);
        }

        for (int i = 50; i < 100; ++i) {
            ht.set(i + 1, &values[i]);
        }

        for (int i = 0; i < 25; ++i) {
            STD_INSIST(ht.find(i + 1) == nullptr);
        }

        for (int i = 25; i < 100; ++i) {
            STD_INSIST(ht.find(i + 1) == &values[i]);
        }
    }

    STD_TEST(eraseSequential) {
        HashTable ht;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        for (int i = 0; i < 10; ++i) {
            ht.erase(i + 1);
            STD_INSIST(ht.find(i + 1) == nullptr);

            for (int j = i + 1; j < 10; ++j) {
                STD_INSIST(ht.find(j + 1) == &values[j]);
            }
        }
    }

    STD_TEST(EraseStressTest) {
        HashTable ht;

        const size_t totalCount = 1000;
        const size_t eraseCount = 500;

        int* values = new int[totalCount];
        for (size_t i = 0; i < totalCount; ++i) {
            values[i] = i * 13;
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.size() == totalCount);

        u64 keys[totalCount];
        for (size_t i = 0; i < totalCount; ++i) {
            keys[i] = i + 1;
        }

        PCG32 rng(42);
        for (size_t i = totalCount - 1; i > 0; --i) {
            size_t j = rng.uniformBiased(i + 1);
            u64 temp = keys[i];
            keys[i] = keys[j];
            keys[j] = temp;
        }

        bool erased[totalCount];
        for (size_t i = 0; i < totalCount; ++i) {
            erased[i] = false;
        }

        for (size_t i = 0; i < eraseCount; ++i) {
            u64 key = keys[i];
            size_t idx = key - 1;

            size_t expectedSize = totalCount - i;
            STD_INSIST(ht.size() == expectedSize);

            void* found = ht.find(key);
            STD_INSIST(found == &values[idx]);

            ht.erase(key);
            erased[idx] = true;

            expectedSize = totalCount - i - 1;
            STD_INSIST(ht.size() == expectedSize);

            STD_INSIST(ht.find(key) == nullptr);

            for (size_t j = 0; j < totalCount; ++j) {
                u64 checkKey = j + 1;
                void* result = ht.find(checkKey);
                if (erased[j]) {
                    STD_INSIST(result == nullptr);
                } else {
                    STD_INSIST(result == &values[j]);
                }
            }
        }

        STD_INSIST(ht.size() == totalCount - eraseCount);

        for (size_t i = 0; i < totalCount; ++i) {
            void* result = ht.find(i + 1);
            if (erased[i]) {
                STD_INSIST(result == nullptr);
            } else {
                STD_INSIST(result == &values[i]);
            }
        }

        delete[] values;
    }

    STD_TEST(CompactifyWithSizeInvariants) {
        HashTable ht;

        const size_t totalCount = 200;
        int* values = new int[totalCount];
        for (size_t i = 0; i < totalCount; ++i) {
            values[i] = i * 7;
        }

        for (size_t i = 0; i < totalCount; ++i) {
            size_t sizeBefore = ht.size();
            ht.set(i + 1, &values[i]);
            STD_INSIST(ht.size() == sizeBefore + 1);
        }

        STD_INSIST(ht.size() == totalCount);

        for (size_t i = 0; i < totalCount; i += 3) {
            size_t sizeBefore = ht.size();
            ht.erase(i + 1);
            STD_INSIST(ht.size() == sizeBefore - 1);
        }

        size_t expectedSize = totalCount - (totalCount + 2) / 3;
        STD_INSIST(ht.size() == expectedSize);

        size_t sizeBeforeCompactify = ht.size();
        size_t capacityBeforeCompactify = ht.capacity();
        ht.compactify();
        STD_INSIST(ht.size() == sizeBeforeCompactify);
        STD_INSIST(ht.capacity() == capacityBeforeCompactify);

        for (size_t i = 0; i < totalCount; ++i) {
            void* found = ht.find(i + 1);
            if (i % 3 == 0) {
                STD_INSIST(found == nullptr);
            } else {
                STD_INSIST(found == &values[i]);
            }
        }

        for (size_t i = 0; i < totalCount; i += 2) {
            if (i % 3 == 0) {
                size_t sizeBefore = ht.size();
                ht.set(i + 1, &values[i]);
                STD_INSIST(ht.size() == sizeBefore + 1);
            }
        }

        for (size_t i = totalCount; i < totalCount + 50; ++i) {
            values[i % totalCount] = i * 11;
            size_t sizeBefore = ht.size();
            ht.set(i + 1, &values[i % totalCount]);
            STD_INSIST(ht.size() == sizeBefore + 1);
        }

        size_t sizeBeforeSecondCompactify = ht.size();
        ht.compactify();
        STD_INSIST(ht.size() == sizeBeforeSecondCompactify);

        for (size_t i = 0; i < totalCount; i += 5) {
            void* foundBefore = ht.find(i + 1);
            if (foundBefore != nullptr) {
                size_t sizeBefore = ht.size();
                ht.erase(i + 1);
                STD_INSIST(ht.size() == sizeBefore - 1);
                STD_INSIST(ht.find(i + 1) == nullptr);
            }
        }

        PCG32 rng(12345);
        for (size_t iteration = 0; iteration < 100; ++iteration) {
            u64 key = rng.uniformBiased(totalCount * 2) + 1;
            void* existing = ht.find(key);

            if (existing != nullptr) {
                size_t sizeBefore = ht.size();
                ht.erase(key);
                STD_INSIST(ht.size() == sizeBefore - 1);
                STD_INSIST(ht.find(key) == nullptr);

                size_t sizeBeforeReinsert = ht.size();
                ht.set(key, &values[key % totalCount]);
                STD_INSIST(ht.size() == sizeBeforeReinsert + 1);
            } else {
                size_t sizeBefore = ht.size();
                ht.set(key, &values[key % totalCount]);
                STD_INSIST(ht.size() == sizeBefore + 1);

                size_t sizeBeforeUpdate = ht.size();
                ht.set(key, &values[(key + 1) % totalCount]);
                STD_INSIST(ht.size() == sizeBeforeUpdate);
            }
        }

        size_t sizeBeforeFinalCompactify = ht.size();
        ht.compactify();
        STD_INSIST(ht.size() == sizeBeforeFinalCompactify);

        struct CountIterator: HashTable::Iterator {
            size_t count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        ht.forEach(it);
        STD_INSIST(it.count == ht.size());

        for (size_t i = 0; i < 50; ++i) {
            u64 key = rng.uniformBiased(totalCount * 3) + 1;
            void* found = ht.find(key);

            if (found != nullptr) {
                size_t sizeBefore = ht.size();
                ht.erase(key);
                STD_INSIST(ht.size() == sizeBefore - 1);
            } else {
                size_t sizeBefore = ht.size();
                ht.erase(key);
                STD_INSIST(ht.size() == sizeBefore);
            }
        }

        delete[] values;
    }
}
