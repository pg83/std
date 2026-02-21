#include "map.h"

#include <std/tst/ut.h>
#include <std/typ/support.h>

using namespace Std;

struct DestructorCounter {
    int* counter;

    DestructorCounter(int* c = nullptr) : counter(c) {}

    DestructorCounter(const DestructorCounter& other) : counter(other.counter) {}

    DestructorCounter& operator=(const DestructorCounter& other) {
        counter = other.counter;
        return *this;
    }

    ~DestructorCounter() {
        if (counter) {
            (*counter)++;
        }
    }
};

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

    STD_TEST(VisitEmptyMap) {
        Map<int, int> m;
        int count = 0;
        m.visit([&count](int, int) {
            count++;
        });
        STD_INSIST(count == 0);
    }

    STD_TEST(VisitSingleElement) {
        Map<int, int> m;
        m[42] = 100;
        int count = 0;
        int foundKey = 0;
        int foundValue = 0;
        m.visit([&](int k, int v) {
            count++;
            foundKey = k;
            foundValue = v;
        });
        STD_INSIST(count == 1);
        STD_INSIST(foundKey == 42);
        STD_INSIST(foundValue == 100);
    }

    STD_TEST(VisitMultipleElements) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;
        m[3] = 30;
        int count = 0;
        int sum = 0;
        m.visit([&](int k, int v) {
            count++;
            sum += v;
        });
        STD_INSIST(count == 3);
        STD_INSIST(sum == 60);
    }

    STD_TEST(VisitInSortedOrder) {
        Map<int, int> m;
        m[5] = 50;
        m[2] = 20;
        m[8] = 80;
        m[1] = 10;
        m[9] = 90;
        int prev = -1;
        m.visit([&](int k, int v) {
            STD_INSIST(k > prev);
            STD_INSIST(k * 10 == v);
            prev = k;
        });
        STD_INSIST(prev == 9);
    }

    STD_TEST(VisitAllKeys) {
        Map<int, int> m;
        for (int i = 0; i < 100; ++i) {
            m[i] = i * 2;
        }
        int count = 0;
        m.visit([&](int k, int v) {
            STD_INSIST(v == k * 2);
            count++;
        });
        STD_INSIST(count == 100);
    }

    STD_TEST(VisitCanModifyValues) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;
        m[3] = 30;
        m.visit([](int k, int& v) {
            v = v * 2;
        });
        STD_INSIST(*m.find(1) == 20);
        STD_INSIST(*m.find(2) == 40);
        STD_INSIST(*m.find(3) == 60);
    }

    STD_TEST(VisitWithNegativeKeys) {
        Map<int, int> m;
        m[-10] = 100;
        m[-5] = 50;
        m[0] = 0;
        m[5] = 50;
        m[10] = 100;
        int count = 0;
        int prev = -100;
        m.visit([&](int k, int v) {
            STD_INSIST(k > prev);
            count++;
            prev = k;
        });
        STD_INSIST(count == 5);
    }

    STD_TEST(VisitAfterInsertAndErase) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;
        m[3] = 30;
        m.erase(2);
        int count = 0;
        m.visit([&](int k, int v) {
            STD_INSIST(k != 2);
            count++;
        });
        STD_INSIST(count == 2);
    }

    STD_TEST(VisitLargeMap) {
        Map<int, int> m;
        for (int i = 0; i < 1000; ++i) {
            m[i] = i * 10;
        }
        int count = 0;
        int prev = -1;
        m.visit([&](int k, int v) {
            STD_INSIST(k == prev + 1);
            STD_INSIST(v == k * 10);
            count++;
            prev = k;
        });
        STD_INSIST(count == 1000);
    }

    STD_TEST(VisitWithDifferentTypes) {
        Map<u32, u64> m;
        m[1] = 100ULL;
        m[2] = 200ULL;
        m[3] = 300ULL;
        u64 sum = 0;
        m.visit([&](u32 k, u64 v) {
            sum += v;
        });
        STD_INSIST(sum == 600ULL);
    }

    STD_TEST(VisitWithPointers) {
        Map<int, int*> m;
        int a = 10, b = 20, c = 30;
        m[1] = &a;
        m[2] = &b;
        m[3] = &c;
        int sum = 0;
        m.visit([&](int k, int* v) {
            sum += *v;
        });
        STD_INSIST(sum == 60);
    }

    STD_TEST(VisitMultipleTimes) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;

        int sum1 = 0;
        m.visit([&](int k, int v) {
            sum1 += v;
        });

        int sum2 = 0;
        m.visit([&](int k, int v) {
            sum2 += v;
        });

        STD_INSIST(sum1 == 30);
        STD_INSIST(sum2 == 30);
    }

    STD_TEST(VisitReverseOrderInsert) {
        Map<int, int> m;
        for (int i = 100; i > 0; --i) {
            m[i] = i * 2;
        }
        int count = 0;
        int expected = 1;
        m.visit([&](int k, int v) {
            STD_INSIST(k == expected);
            STD_INSIST(v == k * 2);
            expected++;
            count++;
        });
        STD_INSIST(count == 100);
    }

    STD_TEST(VisitRandomOrderInsert) {
        Map<int, int> m;
        m[50] = 500;
        m[25] = 250;
        m[75] = 750;
        m[10] = 100;
        m[30] = 300;
        m[60] = 600;
        m[90] = 900;

        int count = 0;
        int prev = -1;
        m.visit([&](int k, int v) {
            STD_INSIST(k > prev);
            STD_INSIST(v == k * 10);
            prev = k;
            count++;
        });
        STD_INSIST(count == 7);
    }

    STD_TEST(VisitAfterUpdate) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;
        m[3] = 30;

        m[2] = 200;

        int sum = 0;
        m.visit([&](int k, int v) {
            sum += v;
        });
        STD_INSIST(sum == 240);
    }

    STD_TEST(VisitCountsCorrectly) {
        Map<int, int> m;
        for (int i = 1; i <= 50; ++i) {
            m[i] = i;
        }

        int count = 0;
        m.visit([&](int, int) {
            count++;
        });

        STD_INSIST(count == 50);
    }

    STD_TEST(VisitWithBoolValues) {
        Map<int, bool> m;
        m[1] = true;
        m[2] = false;
        m[3] = true;

        int trueCount = 0;
        int falseCount = 0;
        m.visit([&](int k, bool v) {
            if (v) {
                trueCount++;
            } else {
                falseCount++;
            }
        });

        STD_INSIST(trueCount == 2);
        STD_INSIST(falseCount == 1);
    }

    STD_TEST(VisitWithCharKeys) {
        Map<char, int> m;
        m['z'] = 26;
        m['a'] = 1;
        m['m'] = 13;

        char prevKey = 0;
        int count = 0;
        m.visit([&](char k, int v) {
            STD_INSIST(k > prevKey);
            prevKey = k;
            count++;
        });

        STD_INSIST(count == 3);
        STD_INSIST(prevKey == 'z');
    }

    STD_TEST(VisitCapturesExternalState) {
        Map<int, int> m;
        m[1] = 10;
        m[2] = 20;
        m[3] = 30;

        int accumulator = 0;
        int multiplier = 2;

        m.visit([&](int k, int v) {
            accumulator += v * multiplier;
        });

        STD_INSIST(accumulator == 120);
    }

    STD_TEST(DestructorCalledOnErase) {
        int destructorCount = 0;

        Map<int, DestructorCounter> m;
        m.insert(1, DestructorCounter(&destructorCount));
        m.insert(2, DestructorCounter(&destructorCount));
        m.insert(3, DestructorCounter(&destructorCount));

        int countBefore = destructorCount;
        m.erase(2);

        STD_INSIST(destructorCount > countBefore);
    }

    STD_TEST(DestructorCalledOnMultipleErases) {
        int destructorCount = 0;

        Map<int, DestructorCounter> m;
        m.insert(1, DestructorCounter(&destructorCount));
        m.insert(2, DestructorCounter(&destructorCount));
        m.insert(3, DestructorCounter(&destructorCount));
        m.insert(4, DestructorCounter(&destructorCount));
        m.insert(5, DestructorCounter(&destructorCount));

        int countBefore = destructorCount;
        m.erase(1);
        int countAfterFirst = destructorCount;
        m.erase(3);
        int countAfterSecond = destructorCount;
        m.erase(5);
        int countAfterThird = destructorCount;

        STD_INSIST(countAfterFirst > countBefore);
        STD_INSIST(countAfterSecond > countAfterFirst);
        STD_INSIST(countAfterThird > countAfterSecond);
    }

    STD_TEST(DestructorCalledWhenMapGoesOutOfScope) {
        int destructorCount = 0;

        {
            Map<int, DestructorCounter> m;
            m.insert(1, DestructorCounter(&destructorCount));
            m.insert(2, DestructorCounter(&destructorCount));
            m.insert(3, DestructorCounter(&destructorCount));
        }

        STD_INSIST(destructorCount > 0);
    }

    STD_TEST(DestructorCalledForAllElementsOnScopeExit) {
        int destructorCount = 0;

        {
            Map<int, DestructorCounter> m;
            for (int i = 0; i < 10; ++i) {
                m.insert(i, DestructorCounter(&destructorCount));
            }
        }

        STD_INSIST(destructorCount >= 10);
    }

    STD_TEST(DestructorCalledOnEraseNonExistentKey) {
        int destructorCount = 0;

        Map<int, DestructorCounter> m;
        m.insert(1, DestructorCounter(&destructorCount));
        m.insert(2, DestructorCounter(&destructorCount));

        int countBefore = destructorCount;
        m.erase(999);

        STD_INSIST(destructorCount == countBefore);
    }

    STD_TEST(DestructorCalledAfterEraseAndScopeExit) {
        int destructorCount = 0;

        {
            Map<int, DestructorCounter> m;
            m.insert(1, DestructorCounter(&destructorCount));
            m.insert(2, DestructorCounter(&destructorCount));
            m.insert(3, DestructorCounter(&destructorCount));
            m.insert(4, DestructorCounter(&destructorCount));
            m.insert(5, DestructorCounter(&destructorCount));

            m.erase(2);
            m.erase(4);
        }

        STD_INSIST(destructorCount >= 5);
    }

    STD_TEST(DestructorCalledForComplexType) {
        int destructorCount = 0;

        {
            Map<int, DestructorCounter> m;
            for (int i = 0; i < 100; ++i) {
                m.insert(i, &destructorCount);
            }

            for (int i = 0; i < 50; ++i) {
                m.erase(i * 2);
            }
        }

        STD_INSIST(destructorCount == 100);
    }
}
