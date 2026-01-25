#include "htable.h"

#include <std/tst/ut.h>

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
        STD_INSIST(ht.getSize() == 3);
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
        STD_INSIST(ht.getSize() == 1);

        ht.set(1, &value2);
        STD_INSIST(ht.find(1) == &value2);
        STD_INSIST(ht.getSize() == 1);
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

        STD_INSIST(ht.getSize() == count);

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

        size_t initialCapacity = ht.getCapacity();

        for (int i = 0; i < 20; ++i) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.getCapacity() > initialCapacity);
        STD_INSIST(ht.getSize() == 20);

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

        STD_INSIST(ht.getSize() == 5);

        STD_INSIST(ht.find(17) == &values[0]);
        STD_INSIST(ht.find(33) == &values[1]);
        STD_INSIST(ht.find(49) == &values[2]);
        STD_INSIST(ht.find(65) == &values[3]);
        STD_INSIST(ht.find(81) == &values[4]);
    }

    STD_TEST(nullptrValues) {
        HashTable ht;

        ht.set(1, nullptr);
        ht.set(2, nullptr);

        STD_INSIST(ht.find(1) == nullptr);
        STD_INSIST(ht.find(2) == nullptr);
        STD_INSIST(ht.getSize() == 2);

        size_t sizeBefore = ht.getSize();
        void* result = ht.find(999);
        STD_INSIST(result == nullptr);
        STD_INSIST(ht.getSize() == sizeBefore);
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
            88888888888888888ULL
        };

        int values[8];
        for (int i = 0; i < 8; ++i) {
            values[i] = i * 100;
            ht.set(keys[i], &values[i]);
        }

        STD_INSIST(ht.getSize() == 8);

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

        STD_INSIST(ht.getSize() == 50);

        for (int i = 1; i < 100; i += 2) {
            ht.set(i + 1, &values[i]);
        }

        STD_INSIST(ht.getSize() == 100);

        for (int i = 0; i < 100; i += 5) {
            ht.set(i + 1, nullptr);
        }

        STD_INSIST(ht.getSize() == 100);

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

        STD_INSIST(ht.getSize() == 0);
        STD_INSIST(ht.getCapacity() == 16);
        STD_INSIST(ht.find(1) == nullptr);
        STD_INSIST(ht.find(999) == nullptr);
    }

    STD_TEST(singleElement) {
        HashTable ht;

        int value = 42;
        ht.set(123, &value);

        STD_INSIST(ht.getSize() == 1);
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
}
