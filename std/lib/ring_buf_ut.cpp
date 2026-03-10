#include "ring_buf.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(RingBuffer) {
    STD_TEST(emptyOnConstruction) {
        void* storage[4];
        RingBuffer rb(storage, 4);

        STD_INSIST(rb.empty());
        STD_INSIST(!rb.full());
        STD_INSIST(rb.size() == 0);
    }

    STD_TEST(pushOneElement) {
        void* storage[4];
        RingBuffer rb(storage, 4);

        int x = 42;
        rb.push(&x);

        STD_INSIST(!rb.empty());
        STD_INSIST(!rb.full());
        STD_INSIST(rb.size() == 1);
    }

    STD_TEST(popReturnsCorrectValue) {
        void* storage[4];
        RingBuffer rb(storage, 4);

        int x = 42;
        rb.push(&x);

        void* v = rb.pop();
        STD_INSIST(v == &x);
        STD_INSIST(rb.empty());
        STD_INSIST(rb.size() == 0);
    }

    STD_TEST(fifoOrder) {
        void* storage[4];
        RingBuffer rb(storage, 4);

        int a = 1, b = 2, c = 3;
        rb.push(&a);
        rb.push(&b);
        rb.push(&c);

        STD_INSIST(rb.pop() == &a);
        STD_INSIST(rb.pop() == &b);
        STD_INSIST(rb.pop() == &c);
    }

    STD_TEST(fullWhenAtCapacity) {
        void* storage[3];
        RingBuffer rb(storage, 3);

        int a = 1, b = 2, c = 3;
        rb.push(&a);
        rb.push(&b);
        rb.push(&c);

        STD_INSIST(rb.full());
        STD_INSIST(rb.size() == 3);
    }

    STD_TEST(wrapAround) {
        void* storage[3];
        RingBuffer rb(storage, 3);

        int a = 1, b = 2, c = 3, d = 4;
        rb.push(&a);
        rb.push(&b);
        rb.push(&c);

        STD_INSIST(rb.pop() == &a);
        STD_INSIST(rb.pop() == &b);

        // tail wraps around
        rb.push(&d);

        STD_INSIST(rb.size() == 2);
        STD_INSIST(rb.pop() == &c);
        STD_INSIST(rb.pop() == &d);
        STD_INSIST(rb.empty());
    }

    STD_TEST(capacityOne) {
        void* storage[1];
        RingBuffer rb(storage, 1);

        int x = 99;
        STD_INSIST(rb.empty());

        rb.push(&x);
        STD_INSIST(rb.full());
        STD_INSIST(rb.size() == 1);

        STD_INSIST(rb.pop() == &x);
        STD_INSIST(rb.empty());
    }

    STD_TEST(repeatPushPop) {
        void* storage[2];
        RingBuffer rb(storage, 2);

        int vals[6] = {10, 20, 30, 40, 50, 60};

        for (int i = 0; i < 6; ++i) {
            rb.push(&vals[i]);
            STD_INSIST(rb.pop() == &vals[i]);
            STD_INSIST(rb.empty());
        }
    }

    STD_TEST(sizeTracking) {
        void* storage[4];
        RingBuffer rb(storage, 4);

        int a = 1, b = 2;
        rb.push(&a);
        STD_INSIST(rb.size() == 1);

        rb.push(&b);
        STD_INSIST(rb.size() == 2);

        rb.pop();
        STD_INSIST(rb.size() == 1);

        rb.pop();
        STD_INSIST(rb.size() == 0);
    }
}
