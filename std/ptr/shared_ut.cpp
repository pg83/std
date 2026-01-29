#include "arc.h"
#include "shared.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>

using namespace Std;

STD_TEST_SUITE(SharedPtr) {
    struct IntValue {
        u32 value = 0;

        IntValue() = default;
        IntValue(u32 v)
            : value(v)
        {
        }
    };

    struct BaseValue {
        u32 baseValue = 42;
        virtual ~BaseValue() = default;
        virtual u32 getValue() const {
            return baseValue;
        }
    };

    struct DerivedValue: public BaseValue {
        u32 derivedValue = 100;
        u32 getValue() const override {
            return derivedValue;
        }
    };

    STD_TEST(testOps) {
        auto v = AtomicSharedPtr<u32>::make(1);

        STD_INSIST(*v == 1);
        STD_INSIST(v.refCount() == 1);

        *v = 0;

        STD_INSIST(*v == 0);
        STD_INSIST(v.refCount() == 1);

        {
            auto c = v;

            *c = 1;

            STD_INSIST(*c == 1);
            STD_INSIST(*v == 1);

            STD_INSIST(v.refCount() == 2);
            STD_INSIST(c.refCount() == 2);

            STD_INSIST(&*c == &*v);
        }

        STD_INSIST(v.refCount() == 1);
        STD_INSIST(*v == 1);
    }

    STD_TEST(testMakeWithArgs) {
        auto v = AtomicSharedPtr<IntValue>::make(42);

        STD_INSIST(v->value == 42);
        STD_INSIST(v.refCount() == 1);

        *v = IntValue(100);
        STD_INSIST(v->value == 100);
    }

    STD_TEST(testCopyConstructor) {
        auto v1 = AtomicSharedPtr<IntValue>::make(10);
        STD_INSIST(v1.refCount() == 1);

        auto v2(v1);
        STD_INSIST(v1.refCount() == 2);
        STD_INSIST(v2.refCount() == 2);
        STD_INSIST(v1.ptr() == v2.ptr());
        STD_INSIST(v1->value == 10);
        STD_INSIST(v2->value == 10);
    }

    STD_TEST(testDereferenceOperators) {
        auto v = AtomicSharedPtr<IntValue>::make(55);

        // Test operator->
        STD_INSIST(v->value == 55);
        v->value = 66;
        STD_INSIST(v->value == 66);

        // Test operator*
        STD_INSIST((*v).value == 66);
        (*v).value = 77;
        STD_INSIST((*v).value == 77);
    }

    STD_TEST(testConstAccess) {
        const auto v = AtomicSharedPtr<IntValue>::make(123);

        STD_INSIST(v.refCount() == 1);
        STD_INSIST(v->value == 123);
        STD_INSIST((*v).value == 123);
    }

    STD_TEST(testMultipleCopies) {
        auto v1 = AtomicSharedPtr<IntValue>::make(10);
        STD_INSIST(v1.refCount() == 1);

        auto v2 = v1;
        STD_INSIST(v1.refCount() == 2);
        STD_INSIST(v2.refCount() == 2);

        auto v3 = v1;
        STD_INSIST(v1.refCount() == 3);
        STD_INSIST(v2.refCount() == 3);
        STD_INSIST(v3.refCount() == 3);

        STD_INSIST(v1.ptr() == v2.ptr());
        STD_INSIST(v2.ptr() == v3.ptr());
    }

    STD_TEST(testScopedLifetime) {
        auto v = AtomicSharedPtr<IntValue>::make(123);
        v->value = 456;

        STD_INSIST(v.refCount() == 1);

        {
            auto c1 = v;
            STD_INSIST(v.refCount() == 2);

            {
                auto c2 = v;
                STD_INSIST(v.refCount() == 3);
                STD_INSIST(c2->value == 456);
            }

            STD_INSIST(v.refCount() == 2);
        }

        STD_INSIST(v.refCount() == 1);
    }

    STD_TEST(testInheritance) {
        auto v = AtomicSharedPtr<DerivedValue>::make();

        STD_INSIST(v.refCount() == 1);
        STD_INSIST(v->getValue() == 100);
        STD_INSIST(v->baseValue == 42);

        v->derivedValue = 200;
        STD_INSIST(v->getValue() == 200);
    }

    STD_TEST(testPointerEquality) {
        auto v1 = AtomicSharedPtr<IntValue>::make(10);
        auto v2 = v1;
        auto v3 = AtomicSharedPtr<IntValue>::make(10);

        STD_INSIST(v1.ptr() == v2.ptr());
        STD_INSIST(v1.ptr() != v3.ptr());
    }

    STD_TEST(testMutPtr) {
        auto v = AtomicSharedPtr<IntValue>::make(10);
        auto* ptr = v.mutPtr();

        ptr->value = 999;
        STD_INSIST(v->value == 999);
    }

    STD_TEST(testPtr) {
        const auto v = AtomicSharedPtr<IntValue>::make(10);
        const auto* ptr = v.ptr();

        STD_INSIST(ptr->value == 10);
    }

    STD_TEST(testRefCountTransitions) {
        auto v = AtomicSharedPtr<IntValue>::make(1);
        STD_INSIST(v.refCount() == 1);

        {
            auto c1 = v;
            STD_INSIST(v.refCount() == 2);

            {
                auto c2 = v;
                STD_INSIST(v.refCount() == 3);

                {
                    auto c3 = v;
                    STD_INSIST(v.refCount() == 4);
                }

                STD_INSIST(v.refCount() == 3);
            }

            STD_INSIST(v.refCount() == 2);
        }

        STD_INSIST(v.refCount() == 1);
    }
}
