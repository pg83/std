#include "arc.h"
#include "intrusive.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(IntrusivePtr) {
    struct Int: public ARC {
        u32 v = 1;
    };

    struct IntWithCtor: public ARC {
        u32 x;
        u32 y;

        IntWithCtor(u32 a, u32 b)
            : x(a)
            , y(b)
        {
        }
    };

    struct Base: public ARC {
        virtual ~Base() = default;
        virtual u32 getValue() const {
            return 42;
        }
    };

    struct Derived: public Base {
        u32 value = 100;

        u32 getValue() const override {
            return value;
        }
    };

    STD_TEST(testOps) {
        auto v = IntrusivePtr<Int>::make();

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->v == 1);

        {
            auto c = v;

            STD_INSIST(v->refCount() == 2);
            STD_INSIST(c->refCount() == 2);

            STD_INSIST(v->v == 1);
            STD_INSIST(c->v == 1);

            c->v = 2;

            STD_INSIST(v->v == 2);
            STD_INSIST(c->v == 2);

            STD_INSIST(&*c == &*v);
        }

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->v == 2);
    }

    STD_TEST(testMakeWithArgs) {
        auto v = IntrusivePtr<IntWithCtor>::make(10, 20);

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->x == 10);
        STD_INSIST(v->y == 20);
    }

    STD_TEST(testDereference) {
        auto v = IntrusivePtr<Int>::make();

        (*v).v = 42;
        STD_INSIST((*v).v == 42);
        STD_INSIST(v->v == 42);
    }

    STD_TEST(testMultipleCopies) {
        auto v1 = IntrusivePtr<Int>::make();
        STD_INSIST(v1->refCount() == 1);

        auto v2 = v1;
        STD_INSIST(v1->refCount() == 2);
        STD_INSIST(v2->refCount() == 2);

        auto v3 = v2;
        STD_INSIST(v1->refCount() == 3);
        STD_INSIST(v2->refCount() == 3);
        STD_INSIST(v3->refCount() == 3);

        STD_INSIST(&*v1 == &*v2);
        STD_INSIST(&*v2 == &*v3);
    }

    STD_TEST(testScopedLifetime) {
        auto v = IntrusivePtr<Int>::make();
        v->v = 123;

        STD_INSIST(v->refCount() == 1);

        {
            auto c1 = v;
            STD_INSIST(v->refCount() == 2);

            {
                auto c2 = v;
                STD_INSIST(v->refCount() == 3);
                STD_INSIST(c2->v == 123);
            }

            STD_INSIST(v->refCount() == 2);
        }

        STD_INSIST(v->refCount() == 1);
    }

    STD_TEST(testConstAccess) {
        const auto v = IntrusivePtr<Int>::make();

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->v == 1);
        STD_INSIST((*v).v == 1);
    }

    STD_TEST(testInheritance) {
        auto v = IntrusivePtr<Derived>::make();

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->getValue() == 100);

        v->value = 200;
        STD_INSIST(v->getValue() == 200);
    }

    STD_TEST(testPointerEquality) {
        auto v1 = IntrusivePtr<Int>::make();
        auto v2 = v1;
        auto v3 = IntrusivePtr<Int>::make();

        STD_INSIST(v1.ptr() == v2.ptr());
        STD_INSIST(v1.ptr() != v3.ptr());
    }

    STD_TEST(testMutPtr) {
        auto v = IntrusivePtr<Int>::make();
        auto* ptr = v.mutPtr();

        ptr->v = 999;
        STD_INSIST(v->v == 999);
    }

    STD_TEST(testRefCountZeroToOne) {
        auto v = IntrusivePtr<Int>::make();

        STD_INSIST(v->refCount() == 1);
    }

    STD_TEST(testChainedCopies) {
        auto v1 = IntrusivePtr<Int>::make();
        v1->v = 7;

        auto v2 = v1;
        auto v3 = v1;
        auto v4 = v2;
        auto v5 = v3;

        STD_INSIST(v1->refCount() == 5);
        STD_INSIST(v2->refCount() == 5);
        STD_INSIST(v3->refCount() == 5);
        STD_INSIST(v4->refCount() == 5);
        STD_INSIST(v5->refCount() == 5);

        STD_INSIST(v1->v == 7);
        STD_INSIST(v2->v == 7);
        STD_INSIST(v3->v == 7);
        STD_INSIST(v4->v == 7);
        STD_INSIST(v5->v == 7);

        v5->v = 15;

        STD_INSIST(v1->v == 15);
        STD_INSIST(v2->v == 15);
        STD_INSIST(v3->v == 15);
        STD_INSIST(v4->v == 15);
        STD_INSIST(v5->v == 15);
    }
}
