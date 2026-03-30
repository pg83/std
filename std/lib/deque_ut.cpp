#include "deque.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(Deque) {
    STD_TEST(EmptyOnConstruction) {
        Deque d;

        STD_INSIST(d.empty());
        STD_INSIST(d.size() == 0);
    }

    STD_TEST(PushPopOne) {
        Deque d;

        int x = 42;
        d.pushBack(&x);

        STD_INSIST(!d.empty());
        STD_INSIST(d.size() == 1);
        STD_INSIST(d.popFront() == &x);
        STD_INSIST(d.empty());
    }

    STD_TEST(FifoOrder) {
        Deque d;

        int a = 1, b = 2, c = 3;
        d.pushBack(&a);
        d.pushBack(&b);
        d.pushBack(&c);

        STD_INSIST(d.popFront() == &a);
        STD_INSIST(d.popFront() == &b);
        STD_INSIST(d.popFront() == &c);
    }

    STD_TEST(RegrowOnFull) {
        Deque d(2);

        int a = 1, b = 2, c = 3, d4 = 4, e = 5;
        d.pushBack(&a);
        d.pushBack(&b);
        d.pushBack(&c);
        d.pushBack(&d4);
        d.pushBack(&e);

        STD_INSIST(d.size() == 5);
        STD_INSIST(d.popFront() == &a);
        STD_INSIST(d.popFront() == &b);
        STD_INSIST(d.popFront() == &c);
        STD_INSIST(d.popFront() == &d4);
        STD_INSIST(d.popFront() == &e);
    }

    STD_TEST(RegrowPreservesOrder) {
        Deque d(4);

        int vals[20];

        for (int i = 0; i < 20; ++i) {
            vals[i] = i;
            d.pushBack(&vals[i]);
        }

        for (int i = 0; i < 20; ++i) {
            STD_INSIST(d.popFront() == &vals[i]);
        }

        STD_INSIST(d.empty());
    }

    STD_TEST(InterleavedPushPop) {
        Deque d(2);

        int a = 1, b = 2, c = 3, d4 = 4;

        d.pushBack(&a);
        d.pushBack(&b);
        STD_INSIST(d.popFront() == &a);

        d.pushBack(&c);
        d.pushBack(&d4);

        STD_INSIST(d.popFront() == &b);
        STD_INSIST(d.popFront() == &c);
        STD_INSIST(d.popFront() == &d4);
        STD_INSIST(d.empty());
    }
}
