#include "scoped.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(ScopedPtr) {
    struct IntValue {
        u32 value = 0;
        static u32 destructorCalls;

        IntValue() = default;
        IntValue(u32 v)
            : value(v)
        {
        }

        ~IntValue() {
            destructorCalls++;
        }
    };

    u32 IntValue::destructorCalls = 0;

    struct BaseValue {
        u32 baseValue = 42;
        static u32 destructorCalls;

        virtual ~BaseValue() {
            destructorCalls++;
        }

        virtual u32 getValue() const {
            return baseValue;
        }
    };

    u32 BaseValue::destructorCalls = 0;

    struct DerivedValue: public BaseValue {
        u32 derivedValue = 100;

        u32 getValue() const override {
            return derivedValue;
        }
    };

    STD_TEST(testBasicConstruction) {
        IntValue::destructorCalls = 0;

        {
            ScopedPtr<IntValue> ptr;
            ptr.ptr = new IntValue(42);

            STD_INSIST(ptr.ptr != nullptr);
            STD_INSIST(ptr.ptr->value == 42);
        }

        STD_INSIST(IntValue::destructorCalls == 1);
    }

    STD_TEST(testDestructor) {
        IntValue::destructorCalls = 0;

        {
            ScopedPtr<IntValue> ptr;
            ptr.ptr = new IntValue(10);
        }

        STD_INSIST(IntValue::destructorCalls == 1);
    }

    STD_TEST(testMultipleDestruction) {
        IntValue::destructorCalls = 0;

        {
            ScopedPtr<IntValue> ptr1;
            ptr1.ptr = new IntValue(1);

            {
                ScopedPtr<IntValue> ptr2;
                ptr2.ptr = new IntValue(2);
            }

            STD_INSIST(IntValue::destructorCalls == 1);
        }

        STD_INSIST(IntValue::destructorCalls == 2);
    }

    STD_TEST(testDrop) {
        IntValue::destructorCalls = 0;

        {
            ScopedPtr<IntValue> ptr;
            ptr.ptr = new IntValue(99);

            STD_INSIST(ptr.ptr != nullptr);

            ptr.drop();

            STD_INSIST(ptr.ptr == nullptr);
        }

        STD_INSIST(IntValue::destructorCalls == 0);
    }

    STD_TEST(testDropPreventsDelete) {
        IntValue::destructorCalls = 0;

        IntValue* rawPtr = new IntValue(123);

        {
            ScopedPtr<IntValue> ptr;
            ptr.ptr = rawPtr;

            ptr.drop();
        }

        STD_INSIST(IntValue::destructorCalls == 0);

        delete rawPtr;

        STD_INSIST(IntValue::destructorCalls == 1);
    }

    STD_TEST(testNullptrDestruction) {
        IntValue::destructorCalls = 0;

        {
            ScopedPtr<IntValue> ptr;
            ptr.ptr = nullptr;
        }

        STD_INSIST(IntValue::destructorCalls == 0);
    }

    STD_TEST(testInheritance) {
        BaseValue::destructorCalls = 0;

        {
            ScopedPtr<DerivedValue> ptr;
            ptr.ptr = new DerivedValue();

            STD_INSIST(ptr.ptr->getValue() == 100);
            STD_INSIST(ptr.ptr->baseValue == 42);
            STD_INSIST(ptr.ptr->derivedValue == 100);
        }

        STD_INSIST(BaseValue::destructorCalls == 1);
    }

    STD_TEST(testPolymorphicDestruction) {
        BaseValue::destructorCalls = 0;

        {
            ScopedPtr<BaseValue> ptr;
            ptr.ptr = new DerivedValue();

            STD_INSIST(ptr.ptr->getValue() == 100);
        }

        STD_INSIST(BaseValue::destructorCalls == 1);
    }

    STD_TEST(testPointerAccess) {
        ScopedPtr<IntValue> ptr;
        ptr.ptr = new IntValue(55);

        STD_INSIST(ptr.ptr->value == 55);

        ptr.ptr->value = 66;

        STD_INSIST(ptr.ptr->value == 66);
    }

    STD_TEST(testModifyThroughPointer) {
        ScopedPtr<IntValue> ptr;
        ptr.ptr = new IntValue(10);

        IntValue* raw = ptr.ptr;
        raw->value = 20;

        STD_INSIST(ptr.ptr->value == 20);
    }

    STD_TEST(testNestedScopes) {
        IntValue::destructorCalls = 0;

        {
            ScopedPtr<IntValue> ptr1;
            ptr1.ptr = new IntValue(1);

            {
                ScopedPtr<IntValue> ptr2;
                ptr2.ptr = new IntValue(2);

                {
                    ScopedPtr<IntValue> ptr3;
                    ptr3.ptr = new IntValue(3);

                    STD_INSIST(IntValue::destructorCalls == 0);
                }

                STD_INSIST(IntValue::destructorCalls == 1);
            }

            STD_INSIST(IntValue::destructorCalls == 2);
        }

        STD_INSIST(IntValue::destructorCalls == 3);
    }

    STD_TEST(testDropAfterMultipleAssignments) {
        IntValue::destructorCalls = 0;

        IntValue* raw1 = new IntValue(1);
        IntValue* raw2 = new IntValue(2);

        {
            ScopedPtr<IntValue> ptr;
            ptr.ptr = raw1;
            ptr.drop();

            STD_INSIST(IntValue::destructorCalls == 0);

            ptr.ptr = raw2;
            ptr.drop();

            STD_INSIST(IntValue::destructorCalls == 0);
        }

        delete raw1;
        delete raw2;

        STD_INSIST(IntValue::destructorCalls == 2);
    }

    STD_TEST(testZeroAfterDrop) {
        ScopedPtr<IntValue> ptr;
        ptr.ptr = new IntValue(100);

        STD_INSIST(ptr.ptr != nullptr);

        ptr.drop();

        STD_INSIST(ptr.ptr == 0);
        STD_INSIST(ptr.ptr == nullptr);
    }
}
