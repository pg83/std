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

        int count = 0;
        ht.visit([&count](void** el) {
            count++;
        });

        STD_INSIST(count == 0);
    }

    STD_TEST(forEachSingleElement) {
        HashTable ht;
        int value = 42;
        ht.set(1, &value);

        int count = 0;
        void** lastEl = nullptr;
        ht.visit([&count, &lastEl](void** el) {
            count++;
            lastEl = el;
        });

        STD_INSIST(count == 1);
        STD_INSIST(lastEl != nullptr);
        STD_INSIST(*lastEl == &value);
    }

    STD_TEST(forEachMultipleElements) {
        HashTable ht;

        int values[5] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        int count = 0;
        ht.visit([&count](void** el) {
            count++;
        });

        STD_INSIST(count == 5);
        STD_INSIST(ht.size() == 5);
    }

    STD_TEST(forEachVisitsAllElements) {
        HashTable ht;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i * 10;
            ht.set(i + 1, &values[i]);
        }

        int sum = 0;
        ht.visit([&sum](void** el) {
            int* ptr = static_cast<int*>(*el);
            sum += *ptr;
        });

        STD_INSIST(sum == 0 + 10 + 20 + 30 + 40 + 50 + 60 + 70 + 80 + 90);
    }

    STD_TEST(forEachModifyValues) {
        HashTable ht;

        int values[5] = {1, 2, 3, 4, 5};
        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        ht.visit([](void** el) {
            int* ptr = static_cast<int*>(*el);
            *ptr *= 2;
        });

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

        size_t visitCount = 0;
        ht.visit([&visitCount](void** el) {
            visitCount++;
        });

        STD_INSIST(visitCount == count);
        STD_INSIST(visitCount == ht.size());

        delete[] values;
    }

    STD_TEST(forEachCollectPointers) {
        HashTable ht;

        int values[5] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; ++i) {
            ht.set(i + 1, &values[i]);
        }

        void* collected[5];
        int index = 0;
        ht.visit([&collected, &index](void** el) {
            if (index < 5) {
                collected[index++] = *el;
            }
        });

        STD_INSIST(index == 5);

        bool found[5] = {false, false, false, false, false};
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) {
                if (collected[i] == &values[j]) {
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

        int count = 0;
        ht.visit([&count](void** el) {
            count++;
        });

        STD_INSIST(count == 20);
        STD_INSIST(count == ht.size());
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

        int sum = 0;
        ht.visit([&sum](void** el) {
            Data* data = static_cast<Data*>(*el);
            sum += data->x;
        });

        STD_INSIST(sum == 60);
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

        int count = 0;
        ht.visit([&count](void** el) {
            count++;
        });

        STD_INSIST(count == 7);
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

    STD_TEST(StressTestRandomOperations) {
        HashTable ht;
        PCG32 rng(0xDEADBEEF);

        const size_t iterations = 10000;
        const size_t keyRange = 1000;
        int values[keyRange * 2];
        void* expectedValues[keyRange];
        bool shouldExist[keyRange];

        for (size_t i = 0; i < keyRange * 2; ++i) {
            values[i] = i * 17;
        }

        for (size_t i = 0; i < keyRange; ++i) {
            expectedValues[i] = nullptr;
            shouldExist[i] = false;
        }

        size_t expectedSize = 0;

        for (size_t iter = 0; iter < iterations; ++iter) {
            u32 op = rng.uniformBiased(100);
            u64 key = rng.uniformBiased(keyRange) + 1;
            size_t idx = key - 1;

            if (op < 40) {
                // ctx.output() << StringView(u8"set ") << key << endL;
                auto prev = ht.set(key, &values[idx]);
                expectedValues[idx] = &values[idx];
                if (!shouldExist[idx]) {
                    STD_INSIST(!prev);
                    shouldExist[idx] = true;
                    expectedSize++;
                } else {
                    STD_INSIST(prev);
                }
            } else if (op < 70) {
                // ctx.output() << StringView(u8"find ") << key << endL;
                void* result = ht.find(key);
                if (shouldExist[idx]) {
                    STD_INSIST(result == expectedValues[idx]);
                } else {
                    STD_INSIST(result == nullptr);
                }
            } else if (op < 90) {
                // ctx.output() << StringView(u8"erase ") << key << endL;
                ht.erase(key);
                if (shouldExist[idx]) {
                    shouldExist[idx] = false;
                    expectedValues[idx] = nullptr;
                    expectedSize--;
                }
            } else {
                if (shouldExist[idx]) {
                    // ctx.output() << StringView(u8"set 2 ") << key << endL;
                    size_t newIdx = (idx + keyRange + iter) % (keyRange * 2);
                    STD_INSIST(ht.set(key, &values[newIdx]));
                    expectedValues[idx] = &values[newIdx];
                }
            }
            size_t cnt = 0;
            ht.visit([&](auto x) {
                ++cnt;
            });
            // ctx.output() << ht.size() << StringView(u8" ") << expectedSize << StringView(u8" ") << cnt << StringView(u8" ") << ht.capacity() << endL;
            STD_INSIST(ht.size() == expectedSize);
            STD_INSIST(cnt == expectedSize);
        }

        for (size_t i = 0; i < keyRange; ++i) {
            void* result = ht.find(i + 1);
            if (shouldExist[i]) {
                STD_INSIST(result == expectedValues[i]);
            } else {
                STD_INSIST(result == nullptr);
            }
        }
    }

    STD_TEST(StressTestSequentialPattern) {
        HashTable ht;

        const size_t rounds = 100;
        const size_t perRound = 200;
        int values[perRound];

        for (size_t i = 0; i < perRound; ++i) {
            values[i] = i;
        }

        for (size_t round = 0; round < rounds; ++round) {
            for (size_t i = 0; i < perRound; ++i) {
                ht.set(i + 1, &values[i]);
            }

            for (size_t i = 0; i < perRound; ++i) {
                STD_INSIST(ht.find(i + 1) == &values[i]);
            }

            for (size_t i = 0; i < perRound; i += 2) {
                ht.erase(i + 1);
            }

            for (size_t i = 0; i < perRound; i += 2) {
                STD_INSIST(ht.find(i + 1) == nullptr);
            }

            for (size_t i = 1; i < perRound; i += 2) {
                STD_INSIST(ht.find(i + 1) == &values[i]);
            }

            for (size_t i = 0; i < perRound; i += 2) {
                ht.set(i + 1, &values[i]);
            }
        }
    }

    STD_TEST(StressTestCollisionHeavy) {
        HashTable ht(32);

        const size_t count = 500;
        int values[count];
        u64 keys[count];

        for (size_t i = 0; i < count; ++i) {
            values[i] = i;
            keys[i] = i * 32;
        }

        for (size_t i = 0; i < count; ++i) {
            ht.set(keys[i], &values[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(ht.find(keys[i]) == &values[i]);
        }

        for (size_t i = 0; i < count; i += 3) {
            ht.erase(keys[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            void* result = ht.find(keys[i]);
            if (i % 3 == 0) {
                STD_INSIST(result == nullptr);
            } else {
                STD_INSIST(result == &values[i]);
            }
        }

        for (size_t i = 0; i < count; i += 3) {
            ht.set(keys[i], &values[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(ht.find(keys[i]) == &values[i]);
        }
    }

    STD_TEST(StressTestAlternatingInsertErase) {
        HashTable ht;
        PCG32 rng(12345);

        const size_t iterations = 5000;
        const size_t keyRange = 500;
        int values[keyRange];

        for (size_t i = 0; i < keyRange; ++i) {
            values[i] = i;
        }

        for (size_t iter = 0; iter < iterations; ++iter) {
            u64 key = rng.uniformBiased(keyRange) + 1;
            size_t idx = key - 1;

            ht.set(key, &values[idx]);
            STD_INSIST(ht.find(key) == &values[idx]);

            ht.erase(key);
            STD_INSIST(ht.find(key) == nullptr);

            ht.set(key, &values[idx]);
            STD_INSIST(ht.find(key) == &values[idx]);
        }
    }

    STD_TEST(StressTestGrowthAndShrinkage) {
        HashTable ht(4);

        const size_t maxSize = 2000;
        int values[maxSize];

        for (size_t i = 0; i < maxSize; ++i) {
            values[i] = i;
        }

        for (size_t phase = 0; phase < 10; ++phase) {
            for (size_t i = 0; i < maxSize; ++i) {
                ht.set(i + 1, &values[i]);
            }

            STD_INSIST(ht.size() == maxSize);

            for (size_t i = 0; i < maxSize; i += 2) {
                ht.erase(i + 1);
            }

            STD_INSIST(ht.size() == maxSize / 2);

            for (size_t i = 0; i < maxSize; i += 2) {
                ht.erase(i + 1 + 1);
            }

            STD_INSIST(ht.size() == 0);
        }
    }

    STD_TEST(StressTestUpdatePattern) {
        HashTable ht;
        PCG32 rng(0xCAFEBABE);

        const size_t keyRange = 800;
        int values[keyRange * 2];

        for (size_t i = 0; i < keyRange * 2; ++i) {
            values[i] = i;
        }

        for (size_t i = 0; i < keyRange; ++i) {
            ht.set(i + 1, &values[i]);
        }

        for (size_t iter = 0; iter < 3000; ++iter) {
            u64 key = rng.uniformBiased(keyRange) + 1;
            size_t newIdx = rng.uniformBiased(keyRange * 2);

            ht.set(key, &values[newIdx]);
            STD_INSIST(ht.find(key) == &values[newIdx]);
        }

        STD_INSIST(ht.size() == keyRange);
    }

    STD_TEST(StressTestFindAfterManyErases) {
        HashTable ht;
        PCG32 rng(777);

        const size_t totalKeys = 1000;
        int values[totalKeys];

        for (size_t i = 0; i < totalKeys; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        u64 keys[totalKeys];
        for (size_t i = 0; i < totalKeys; ++i) {
            keys[i] = i + 1;
        }

        for (size_t i = totalKeys - 1; i > 0; --i) {
            size_t j = rng.uniformBiased(i + 1);
            u64 temp = keys[i];
            keys[i] = keys[j];
            keys[j] = temp;
        }

        for (size_t i = 0; i < totalKeys * 3 / 4; ++i) {
            ht.erase(keys[i]);
        }

        for (size_t trial = 0; trial < 5000; ++trial) {
            u64 key = rng.uniformBiased(totalKeys) + 1;
            ht.find(key);
        }

        for (size_t i = 0; i < totalKeys; ++i) {
            void* result = ht.find(i + 1);
            bool shouldExist = false;
            for (size_t j = totalKeys * 3 / 4; j < totalKeys; ++j) {
                if (keys[j] == i + 1) {
                    shouldExist = true;
                    break;
                }
            }

            if (shouldExist) {
                STD_INSIST(result == &values[i]);
            } else {
                STD_INSIST(result == nullptr);
            }
        }
    }

    STD_TEST(StressTestDenseKeySpace) {
        HashTable ht;

        const size_t count = 1500;
        int values[count];

        for (size_t i = 0; i < count; ++i) {
            values[i] = i;
            ht.set(i + 1, &values[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(ht.find(i + 1) == &values[i]);
        }

        for (size_t i = 0; i < count; i += 5) {
            ht.erase(i + 1);
        }

        for (size_t i = 0; i < count; i += 5) {
            STD_INSIST(ht.find(i + 1) == nullptr);
        }

        for (size_t i = 0; i < count; i += 5) {
            ht.set(i + 1, &values[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(ht.find(i + 1) == &values[i]);
        }
    }

    STD_TEST(StressTestSparseKeySpace) {
        HashTable ht;
        PCG32 rng(0x123456);

        const size_t count = 800;
        int values[count];
        u64 keys[count];

        for (size_t i = 0; i < count; ++i) {
            values[i] = i;
            keys[i] = ((u64)rng.nextU32() << 32) | rng.nextU32();
            if (keys[i] == 0) {
                keys[i] = 1;
            }
        }

        for (size_t i = 0; i < count; ++i) {
            ht.set(keys[i], &values[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(ht.find(keys[i]) == &values[i]);
        }

        for (size_t i = 0; i < count; i += 4) {
            ht.erase(keys[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            void* result = ht.find(keys[i]);
            if (i % 4 == 0) {
                STD_INSIST(result == nullptr);
            } else {
                STD_INSIST(result == &values[i]);
            }
        }
    }

    STD_TEST(StressTestFullTableRecovery) {
        HashTable ht(16);

        const size_t capacity = ht.capacity();
        const size_t fillCount = capacity * 0.7 - 1;

        int values[capacity];
        for (size_t i = 0; i < capacity; ++i) {
            values[i] = i;
        }

        for (size_t i = 0; i < fillCount; ++i) {
            ht.set(i + 1, &values[i]);
        }

        for (size_t i = 0; i < fillCount / 2; ++i) {
            ht.erase(i + 1);
        }

        for (size_t i = 0; i < fillCount / 2; ++i) {
            ht.set(i + 1, &values[i]);
        }

        for (size_t i = 0; i < fillCount; ++i) {
            STD_INSIST(ht.find(i + 1) == &values[i]);
        }
    }

    STD_TEST(StressTestMixedPatterns) {
        HashTable ht;
        PCG32 rng(0xABCDEF);

        const size_t keyRange = 1000;
        int values[keyRange];

        for (size_t i = 0; i < keyRange; ++i) {
            values[i] = i;
        }

        for (size_t phase = 0; phase < 5; ++phase) {
            for (size_t i = 0; i < keyRange; i += 2) {
                ht.set(i + 1, &values[i]);
            }

            for (size_t i = 1; i < keyRange; i += 2) {
                ht.set(i + 1, &values[i]);
            }

            for (size_t i = 0; i < 200; ++i) {
                u64 key = rng.uniformBiased(keyRange) + 1;
                ht.erase(key);
            }

            for (size_t i = 0; i < 200; ++i) {
                u64 key = rng.uniformBiased(keyRange) + 1;
                ht.set(key, &values[key - 1]);
            }

            for (size_t i = 0; i < 500; ++i) {
                u64 key = rng.uniformBiased(keyRange) + 1;
                ht.find(key);
            }

            for (size_t i = 0; i < keyRange; i += 3) {
                ht.erase(i + 1);
            }
        }
    }

    STD_TEST(StressTestRehashDuringOperations) {
        HashTable ht(4);
        PCG32 rng(54321);

        const size_t iterations = 2000;
        const size_t keyRange = 300;
        int values[keyRange];

        for (size_t i = 0; i < keyRange; ++i) {
            values[i] = i;
        }

        size_t insertCount = 0;
        size_t eraseCount = 0;

        for (size_t iter = 0; iter < iterations; ++iter) {
            if (rng.uniformBiased(100) < 60) {
                u64 key = rng.uniformBiased(keyRange) + 1;
                ht.set(key, &values[key - 1]);
                insertCount++;
            } else {
                u64 key = rng.uniformBiased(keyRange) + 1;
                ht.erase(key);
                eraseCount++;
            }

            if (iter % 100 == 0) {
                for (size_t verify = 0; verify < 50; ++verify) {
                    u64 key = rng.uniformBiased(keyRange) + 1;
                    ht.find(key);
                }
            }
        }
    }

    STD_TEST(StressTestDeepCollisionChains) {
        HashTable ht(64);

        const size_t capacity = ht.capacity();
        const size_t chainLength = 30;
        int values[chainLength];

        for (size_t i = 0; i < chainLength; ++i) {
            values[i] = i;
            u64 key = i * capacity;
            ht.set(key, &values[i]);
        }

        for (size_t i = 0; i < chainLength; ++i) {
            u64 key = i * capacity;
            STD_INSIST(ht.find(key) == &values[i]);
        }

        for (size_t i = 0; i < chainLength; i += 3) {
            u64 key = i * capacity;
            ht.erase(key);
        }

        for (size_t i = 0; i < chainLength; ++i) {
            u64 key = i * capacity;
            void* result = ht.find(key);
            if (i % 3 == 0) {
                STD_INSIST(result == nullptr);
            } else {
                STD_INSIST(result == &values[i]);
            }
        }

        for (size_t i = 0; i < chainLength; i += 3) {
            u64 key = i * capacity;
            ht.set(key, &values[i]);
        }

        for (size_t i = 0; i < chainLength; ++i) {
            u64 key = i * capacity;
            STD_INSIST(ht.find(key) == &values[i]);
        }
    }

    STD_TEST(StressTestHeavyLoad) {
        HashTable ht;
        PCG32 rng(0xFEEDBEEF);

        const size_t keyRange = 2000;
        const size_t operations = 20000;
        int values[keyRange];

        for (size_t i = 0; i < keyRange; ++i) {
            values[i] = i;
        }

        for (size_t op = 0; op < operations; ++op) {
            u32 choice = rng.uniformBiased(100);
            u64 key = rng.uniformBiased(keyRange) + 1;
            size_t idx = key - 1;

            if (choice < 50) {
                ht.set(key, &values[idx]);
            } else if (choice < 80) {
                ht.find(key);
            } else if (choice < 95) {
                ht.erase(key);
            } else {
                ht.erase(key);
            }
        }

        for (size_t verify = 0; verify < 1000; ++verify) {
            u64 key = rng.uniformBiased(keyRange) + 1;
            ht.find(key);
        }
    }
}
