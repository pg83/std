#include "map.h"

#include <std/tst/ut.h>
#include <std/typ/support.h>

using namespace Std;

STD_TEST_SUITE(Map) {
    STD_TEST(DefaultConstruction) {
        Map<int, int> m;
        STD_INSIST(m.find(42) == nullptr);
    }

    STD_TEST(InsertAndFind) {
        Map<int, int> m;
        int* val = m.insert(10, 100);
        STD_INSIST(val != nullptr);
        STD_INSIST(*val == 100);

        int* found = m.find(10);
        STD_INSIST(found != nullptr);
        STD_INSIST(*found == 100);
    }

    STD_TEST(InsertMultiple) {
        Map<int, int> m;
        m.insert(1, 10);
        m.insert(2, 20);
        m.insert(3, 30);

        STD_INSIST(*m.find(1) == 10);
        STD_INSIST(*m.find(2) == 20);
        STD_INSIST(*m.find(3) == 30);
    }

    STD_TEST(FindNonExistent) {
        Map<int, int> m;
        m.insert(1, 10);
        m.insert(2, 20);

        STD_INSIST(m.find(3) == nullptr);
        STD_INSIST(m.find(99) == nullptr);
    }

    STD_TEST(OperatorBracketInsert) {
        Map<int, int> m;
        m[42] = 100;
        STD_INSIST(*m.find(42) == 100);
    }

    STD_TEST(OperatorBracketUpdate) {
        Map<int, int> m;
        m[42] = 100;
        STD_INSIST(*m.find(42) == 100);
        m[42] = 200;
        STD_INSIST(*m.find(42) == 200);
    }

    STD_TEST(OperatorBracketMultiple) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;
        m[3] = 30;

        STD_INSIST(m[1] == 10);
        STD_INSIST(m[2] == 20);
        STD_INSIST(m[3] == 30);
    }

    STD_TEST(MixedInsertAndBracket) {
        Map<int, int> m;
        m.insert(1, 10);
        m[2] = 20;
        m.insert(3, 30);

        STD_INSIST(*m.find(1) == 10);
        STD_INSIST(m[2] == 20);
        STD_INSIST(*m.find(3) == 30);
    }

    STD_TEST(InsertZeroKey) {
        Map<int, int> m;
        m.insert(0, 100);
        STD_INSIST(*m.find(0) == 100);
    }

    STD_TEST(InsertNegativeKey) {
        Map<int, int> m;
        m.insert(-10, 100);
        m.insert(-5, 50);
        STD_INSIST(*m.find(-10) == 100);
        STD_INSIST(*m.find(-5) == 50);
    }

    STD_TEST(LargeNumberOfInserts) {
        Map<int, int> m;
        for (int i = 0; i < 1000; ++i) {
            m.insert(i, i * 10);
        }

        for (int i = 0; i < 1000; ++i) {
            int* val = m.find(i);
            STD_INSIST(val != nullptr);
            STD_INSIST(*val == i * 10);
        }
    }

    STD_TEST(UpdateValue) {
        Map<int, int> m;
        int* val = m.insert(10, 100);
        STD_INSIST(*val == 100);
        *val = 200;
        STD_INSIST(*m.find(10) == 200);
    }

    STD_TEST(UpdateViaBracket) {
        Map<int, int> m;
        m[10] = 100;
        m[10] = 200;
        STD_INSIST(m[10] == 200);
    }

    STD_TEST(StringKeys) {
        Map<u32, u32> m;
        m.insert(123, 456);
        m.insert(789, 101112);
        STD_INSIST(*m.find(123) == 456);
        STD_INSIST(*m.find(789) == 101112);
    }

    STD_TEST(UnsignedIntKeys) {
        Map<u32, u32> m;
        m.insert(0, 10);
        m.insert(100, 20);
        m.insert(4294967295u, 30);

        STD_INSIST(*m.find(0) == 10);
        STD_INSIST(*m.find(100) == 20);
        STD_INSIST(*m.find(4294967295u) == 30);
    }

    STD_TEST(InsertAndFindPointers) {
        Map<int, int*> m;
        int x = 100;
        int y = 200;
        m.insert(1, &x);
        m.insert(2, &y);

        int** found1 = m.find(1);
        int** found2 = m.find(2);

        STD_INSIST(found1 != nullptr);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(*found1 == &x);
        STD_INSIST(*found2 == &y);
        STD_INSIST(**found1 == 100);
        STD_INSIST(**found2 == 200);
    }

    STD_TEST(OperatorBracketReturnsReference) {
        Map<int, int> m;
        m[10] = 100;
        int& ref = m[10];
        ref = 200;
        STD_INSIST(m[10] == 200);
    }

    STD_TEST(FindReturnsNullForEmpty) {
        Map<int, int> m;
        STD_INSIST(m.find(1) == nullptr);
        STD_INSIST(m.find(0) == nullptr);
        STD_INSIST(m.find(-1) == nullptr);
    }

    STD_TEST(InsertSameKeyMultipleTimes) {
        Map<int, int> m;
        m.insert(10, 100);
        m.insert(10, 200);
        m.insert(10, 300);

        int* val = m.find(10);
        STD_INSIST(val != nullptr);
        STD_INSIST(*val == 300);
    }

    STD_TEST(ManyKeysRandomOrder) {
        Map<int, int> m;
        m.insert(50, 500);
        m.insert(20, 200);
        m.insert(80, 800);
        m.insert(10, 100);
        m.insert(30, 300);
        m.insert(70, 700);
        m.insert(90, 900);

        STD_INSIST(*m.find(50) == 500);
        STD_INSIST(*m.find(20) == 200);
        STD_INSIST(*m.find(80) == 800);
        STD_INSIST(*m.find(10) == 100);
        STD_INSIST(*m.find(30) == 300);
        STD_INSIST(*m.find(70) == 700);
        STD_INSIST(*m.find(90) == 900);
    }

    STD_TEST(BracketOperatorChaining) {
        Map<int, int> m;
        m[1] = m[2] = m[3] = 100;
        STD_INSIST(m[1] == 100);
        STD_INSIST(m[2] == 100);
        STD_INSIST(m[3] == 100);
    }

    STD_TEST(InsertWithForwarding) {
        Map<int, int> m;
        int value = 42;
        m.insert(1, value);
        STD_INSIST(*m.find(1) == 42);
    }

    STD_TEST(EmptyMapOperations) {
        Map<int, int> m;
        STD_INSIST(m.find(0) == nullptr);
        STD_INSIST(m.find(1) == nullptr);
        STD_INSIST(m.find(-1) == nullptr);
    }

    STD_TEST(SingleElementMap) {
        Map<int, int> m;
        m[42] = 84;
        STD_INSIST(*m.find(42) == 84);
        STD_INSIST(m.find(41) == nullptr);
        STD_INSIST(m.find(43) == nullptr);
    }

    STD_TEST(OverwriteViaBracket) {
        Map<int, int> m;
        m[10] = 100;
        m[10] = 200;
        m[10] = 300;
        STD_INSIST(m[10] == 300);
    }

    STD_TEST(ZeroValue) {
        Map<int, int> m;
        m.insert(1, 0);
        m.insert(2, 0);
        STD_INSIST(*m.find(1) == 0);
        STD_INSIST(*m.find(2) == 0);
    }

    STD_TEST(NegativeValues) {
        Map<int, int> m;
        m.insert(1, -100);
        m.insert(2, -200);
        STD_INSIST(*m.find(1) == -100);
        STD_INSIST(*m.find(2) == -200);
    }

    STD_TEST(MixedPositiveNegativeKeys) {
        Map<int, int> m;
        m.insert(-10, 100);
        m.insert(10, 200);
        m.insert(-5, 300);
        m.insert(5, 400);
        m.insert(0, 500);

        STD_INSIST(*m.find(-10) == 100);
        STD_INSIST(*m.find(10) == 200);
        STD_INSIST(*m.find(-5) == 300);
        STD_INSIST(*m.find(5) == 400);
        STD_INSIST(*m.find(0) == 500);
    }

    STD_TEST(PointerValues) {
        Map<int, void*> m;
        int a = 10;
        int b = 20;
        m.insert(1, &a);
        m.insert(2, &b);

        void** found1 = m.find(1);
        void** found2 = m.find(2);

        STD_INSIST(found1 != nullptr);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(*found1 == &a);
        STD_INSIST(*found2 == &b);
    }

    STD_TEST(BracketOperatorCreatesEntry) {
        Map<int, int> m;
        int& ref = m[42];
        ref = 100;
        STD_INSIST(m[42] == 100);
    }

    STD_TEST(FindConstCorrectness) {
        Map<int, int> m;
        m.insert(10, 100);
        const Map<int, int>& cm = m;
        const int* val = cm.find(10);
        STD_INSIST(val != nullptr);
        STD_INSIST(*val == 100);
    }

    STD_TEST(MultipleTypes) {
        Map<u64, u8> m;
        m.insert(1000000000000ULL, 255);
        STD_INSIST(*m.find(1000000000000ULL) == 255);
    }

    STD_TEST(BooleanValues) {
        Map<int, bool> m;
        m.insert(1, true);
        m.insert(2, false);
        STD_INSIST(*m.find(1) == true);
        STD_INSIST(*m.find(2) == false);
    }

    STD_TEST(CharKeys) {
        Map<char, int> m;
        m.insert('a', 1);
        m.insert('b', 2);
        m.insert('z', 26);
        STD_INSIST(*m.find('a') == 1);
        STD_INSIST(*m.find('b') == 2);
        STD_INSIST(*m.find('z') == 26);
    }

    STD_TEST(SequentialInserts) {
        Map<int, int> m;
        for (int i = 1; i <= 100; ++i) {
            m.insert(i, i * i);
        }

        for (int i = 1; i <= 100; ++i) {
            STD_INSIST(*m.find(i) == i * i);
        }

        STD_INSIST(m.find(0) == nullptr);
        STD_INSIST(m.find(101) == nullptr);
    }

    STD_TEST(ReverseSequentialInserts) {
        Map<int, int> m;
        for (int i = 100; i >= 1; --i) {
            m.insert(i, i * 10);
        }

        for (int i = 1; i <= 100; ++i) {
            STD_INSIST(*m.find(i) == i * 10);
        }
    }

    STD_TEST(UpdateAfterFind) {
        Map<int, int> m;
        m.insert(10, 100);
        int* val = m.find(10);
        STD_INSIST(val != nullptr);
        *val = 200;
        STD_INSIST(*m.find(10) == 200);
    }

    STD_TEST(BracketOperatorDefaultValue) {
        Map<int, int> m;
        int& val = m[99];
        STD_INSIST(m.find(99) != nullptr);
    }

    STD_TEST(MultipleMapInstances) {
        Map<int, int> m1;
        Map<int, int> m2;

        m1.insert(1, 10);
        m2.insert(1, 20);

        STD_INSIST(*m1.find(1) == 10);
        STD_INSIST(*m2.find(1) == 20);
    }

    STD_TEST(DifferentKeyTypes) {
        Map<u32, int> m;
        m.insert(100u, 200);
        STD_INSIST(*m.find(100u) == 200);
    }

    STD_TEST(StressTestInserts) {
        Map<int, int> m;
        for (int i = 0; i < 10000; ++i) {
            m.insert(i, i * 2);
        }

        for (int i = 0; i < 10000; ++i) {
            int* val = m.find(i);
            STD_INSIST(val != nullptr);
            STD_INSIST(*val == i * 2);
        }
    }

    STD_TEST(InsertReturnValue) {
        Map<int, int> m;
        int* p1 = m.insert(1, 10);
        int* p2 = m.insert(2, 20);
        STD_INSIST(p1 != nullptr);
        STD_INSIST(p2 != nullptr);
        STD_INSIST(p1 != p2);
        STD_INSIST(*p1 == 10);
        STD_INSIST(*p2 == 20);
    }

    STD_TEST(BracketOperatorReturnsCorrectReference) {
        Map<int, int> m;
        m[1] = 10;
        int& ref1 = m[1];
        int& ref2 = m[1];
        STD_INSIST(&ref1 == &ref2);
        STD_INSIST(ref1 == 10);
    }

    STD_TEST(VisitorX) {
        Map<int, int> m;
        m[1] = 10;
        m.visit([](int, int) {
        });
    }
}
