#include "singleton.h"

#include <std/tst/ut.h>

using namespace Std;

namespace {
    // Test classes for singleton with unique names for each test
    template <int ID>
    struct TestSingleton {
        int value = 42 + ID;
        static int constructorCalls;

        TestSingleton() {
            ++constructorCalls;
        }
    };

    template <int ID>
    int TestSingleton<ID>::constructorCalls = 0;

    // Helper function to reset constructor call counter for specific ID
    template <int ID>
    static void resetCounter() {
        TestSingleton<ID>::constructorCalls = 0;
    }
}

STD_TEST_SUITE(Singleton) {
    STD_TEST(SingletonReturnsSameInstance) {
        resetCounter<1>();
        TestSingleton<1>& instance1 = singleton<TestSingleton<1>>();
        TestSingleton<1>& instance2 = singleton<TestSingleton<1>>();

        STD_INSIST(&instance1 == &instance2);
    }

    STD_TEST(SingletonInitializesOnce) {
        resetCounter<2>();

        TestSingleton<2>& instance1 = singleton<TestSingleton<2>>();
        TestSingleton<2>& instance2 = singleton<TestSingleton<2>>();

        STD_INSIST(TestSingleton<2>::constructorCalls == 1);
        STD_INSIST(&instance1 == &instance2);
    }

    STD_TEST(SingletonStoresCorrectValue) {
        resetCounter<3>();
        TestSingleton<3>& instance = singleton<TestSingleton<3>>();

        STD_INSIST(instance.value == 45); // 42 + 3
    }

    STD_TEST(SingletonModifiesValue) {
        resetCounter<4>();
        TestSingleton<4>& instance = singleton<TestSingleton<4>>();
        instance.value = 100;

        TestSingleton<4>& instance2 = singleton<TestSingleton<4>>();

        STD_INSIST(instance2.value == 100);
    }

    STD_TEST(DifferentSingletonTypes) {
        resetCounter<5>();
        resetCounter<6>();
        TestSingleton<5>& testInstance = singleton<TestSingleton<5>>();
        TestSingleton<6>& anotherInstance = singleton<TestSingleton<6>>();

        STD_INSIST(testInstance.value == 47);    // 42 + 5
        STD_INSIST(anotherInstance.value == 48); // 42 + 6
    }

    STD_TEST(SingletonConstructorCalledOncePerType) {
        resetCounter<7>();
        resetCounter<8>();

        TestSingleton<7>& testInstance1 = singleton<TestSingleton<7>>();
        TestSingleton<7>& testInstance2 = singleton<TestSingleton<7>>();
        TestSingleton<8>& anotherInstance1 = singleton<TestSingleton<8>>();
        TestSingleton<8>& anotherInstance2 = singleton<TestSingleton<8>>();

        STD_INSIST(TestSingleton<7>::constructorCalls == 1);
        STD_INSIST(TestSingleton<8>::constructorCalls == 1);
        STD_INSIST(&testInstance1 == &testInstance2);
        STD_INSIST(&anotherInstance1 == &anotherInstance2);
    }

    STD_TEST(SingletonWithModifiedValues) {
        resetCounter<9>();
        resetCounter<10>();
        TestSingleton<9>& instance = singleton<TestSingleton<9>>();
        instance.value = 200;

        TestSingleton<10>& anotherInstance = singleton<TestSingleton<10>>();
        anotherInstance.value = 271; // 2.71 * 100 for integer

        TestSingleton<9>& instance2 = singleton<TestSingleton<9>>();
        TestSingleton<10>& anotherInstance2 = singleton<TestSingleton<10>>();

        STD_INSIST(instance2.value == 200);
        STD_INSIST(anotherInstance2.value == 271);
    }

    STD_TEST(SingletonTypeSafety) {
        resetCounter<11>();
        TestSingleton<11>& instance = singleton<TestSingleton<11>>();
        TestSingleton<11>* ptr = &instance;

        TestSingleton<11>& instance2 = singleton<TestSingleton<11>>();
        TestSingleton<11>* ptr2 = &instance2;

        STD_INSIST(ptr == ptr2);
    }

    STD_TEST(SingletonMultipleAccesses) {
        resetCounter<12>();
        TestSingleton<12>* ptr = nullptr;
        for (int i = 0; i < 10; ++i) {
            TestSingleton<12>& instance = singleton<TestSingleton<12>>();
            if (i == 0) {
                ptr = &instance;
            } else {
                STD_INSIST(ptr == &instance);
            }
        }
    }

    STD_TEST(SingletonDifferentTypesIsolation) {
        resetCounter<13>();
        resetCounter<14>();

        TestSingleton<13>& testInstance = singleton<TestSingleton<13>>();
        TestSingleton<14>& anotherInstance = singleton<TestSingleton<14>>();

        STD_INSIST(TestSingleton<13>::constructorCalls == 1);
        STD_INSIST(TestSingleton<14>::constructorCalls == 1);
        STD_INSIST(testInstance.value == 55);    // 42 + 13
        STD_INSIST(anotherInstance.value == 56); // 42 + 14
    }
}
