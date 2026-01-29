#include "buffer.h"

#include <std/tst/ut.h>
#include <std/sys/crt.h>

using namespace Std;

STD_TEST_SUITE(Buffer) {
    STD_TEST(reset1) {
        Buffer b;

        b.reset();

        STD_INSIST(b.empty());
    }

    STD_TEST(default_constructor) {
        Buffer b;

        STD_INSIST(b.empty());
        STD_INSIST(b.used() == 0);
        STD_INSIST(b.length() == 0);
        STD_INSIST(b.data() != nullptr);
    }

    STD_TEST(constructor_with_size) {
        const size_t size = 100;
        Buffer b(size);

        STD_INSIST(b.capacity() >= size);
        STD_INSIST(b.empty());
        STD_INSIST(b.used() == 0);
        STD_INSIST(b.left() >= size);
    }

    STD_TEST(constructor_with_data) {
        const char* testData = "Hello, World!";
        const size_t len = 13;
        Buffer b(testData, len);

        STD_INSIST(b.used() == len);
        STD_INSIST(b.capacity() >= len);
        STD_INSIST(!b.empty());
        STD_INSIST(memCmp(b.data(), testData, len) == 0);
    }

    STD_TEST(copy_constructor) {
        const char* testData = "Test data";
        const size_t len = 9;
        Buffer b1(testData, len);
        Buffer b2(b1);

        STD_INSIST(b2.used() == b1.used());
        STD_INSIST(b2.capacity() >= b1.capacity());
        STD_INSIST(memCmp(b1.data(), b2.data(), len) == 0);
        STD_INSIST(b1.data() != b2.data()); // deep copy
    }

    STD_TEST(move_constructor) {
        const char* testData = "Move test";
        const size_t len = 9;
        Buffer b1(testData, len);
        void* originalData = b1.mutData();
        size_t originalUsed = b1.used();

        Buffer b2(move(b1));

        STD_INSIST(b2.used() == originalUsed);
        STD_INSIST(b2.data() == originalData);
        STD_INSIST(b1.empty()); // b1 should be empty after move
    }

    STD_TEST(copy_assignment) {
        Buffer b1("data1", 5);
        Buffer b2("data2", 5);

        b2 = b1;

        STD_INSIST(b2.used() == b1.used());
        STD_INSIST(memCmp(b1.data(), b2.data(), b1.used()) == 0);
    }

    STD_TEST(move_assignment) {
        Buffer b1("data1", 5);
        Buffer b2("data2", 5);
        size_t originalUsed = b1.used();

        b2 = move(b1);

        STD_INSIST(b2.used() == originalUsed);
        STD_INSIST(b1.empty());
    }

    STD_TEST(reset) {
        Buffer b("test", 4);

        STD_INSIST(!b.empty());
        b.reset();

        STD_INSIST(b.empty());
        STD_INSIST(b.used() == 0);
    }

    STD_TEST(empty) {
        Buffer b;
        STD_INSIST(b.empty());

        b.append("x", 1);
        STD_INSIST(!b.empty());

        b.reset();
        STD_INSIST(b.empty());
    }

    STD_TEST(capacity_and_left) {
        Buffer b(100);
        size_t cap = b.capacity();

        STD_INSIST(b.left() == cap);

        b.append("test", 4);
        STD_INSIST(b.left() == cap - 4);
    }

    STD_TEST(seekAbsolute_position) {
        Buffer b(100);

        b.seekAbsolute(50);
        STD_INSIST(b.used() == 50);

        b.seekAbsolute((size_t)0);
        STD_INSIST(b.used() == 0);

        b.seekAbsolute(100);
        STD_INSIST(b.used() == 100);
    }

    STD_TEST(seekAbsolute_pointer) {
        Buffer b(100);
        void* ptr = (void*)((u8*)b.mutData() + 25);

        b.seekAbsolute(ptr);
        STD_INSIST(b.used() == 25);
    }

    STD_TEST(seekRelative) {
        Buffer b(100);

        b.seekRelative(10);
        STD_INSIST(b.used() == 10);

        b.seekRelative(5);
        STD_INSIST(b.used() == 15);

        b.reset();
        b.seekRelative(20);
        STD_INSIST(b.used() == 20);
    }

    STD_TEST(current_pointer) {
        Buffer b(100);

        STD_INSIST(b.current() == b.data());

        b.seekRelative(10);
        const void* expectedCurrent = advancePtr(b.data(), 10);
        STD_INSIST(b.current() == expectedCurrent);
    }

    STD_TEST(storageEnd_pointer) {
        Buffer b(100);

        const void* end = b.storageEnd();
        const void* expectedEnd = advancePtr(b.data(), b.capacity());

        STD_INSIST(end == expectedEnd);
    }

    STD_TEST(offsetOf) {
        Buffer b(100);
        void* ptr = advancePtr(b.mutData(), 42);

        size_t offset = b.offsetOf(ptr);
        STD_INSIST(offset == 42);
    }

    STD_TEST(append) {
        Buffer b(100);
        const char* data1 = "Hello";
        const char* data2 = " World";

        b.append(data1, 5);
        STD_INSIST(b.used() == 5);
        STD_INSIST(memCmp(b.data(), "Hello", 5) == 0);

        b.append(data2, 6);
        STD_INSIST(b.used() == 11);
        STD_INSIST(memCmp(b.data(), "Hello World", 11) == 0);
    }

    STD_TEST(append_with_grow) {
        Buffer b(5);
        auto data = u8"This is a long string that exceeds initial capacity";
        size_t len = strLen(data);

        b.append(data, len);

        STD_INSIST(b.used() == len);
        STD_INSIST(b.capacity() >= len);
        STD_INSIST(memCmp(b.data(), data, len) == 0);
    }

    STD_TEST(grow) {
        Buffer b(10);
        size_t oldCapacity = b.capacity();

        b.grow(100);

        STD_INSIST(b.capacity() >= 100);
        STD_INSIST(b.capacity() > oldCapacity);
    }

    STD_TEST(growDelta) {
        Buffer b(10);
        b.append("test", 4);

        b.growDelta(50);

        STD_INSIST(b.capacity() >= 54); // 4 + 50
    }

    STD_TEST(shrinkToFit) {
        Buffer b(1000);
        b.append("small", 5);

        b.shrinkToFit();

        STD_INSIST(b.capacity() >= b.used());
        STD_INSIST(b.used() == 5);
        STD_INSIST(memCmp(b.data(), "small", 5) == 0);
    }

    STD_TEST(xchg) {
        Buffer b1("data1", 5);
        Buffer b2("data2", 5);

        void* data1 = b1.mutData();
        void* data2 = b2.mutData();

        b1.xchg(b2);

        STD_INSIST(b1.data() == data2);
        STD_INSIST(b2.data() == data1);
    }

    STD_TEST(mutData_and_mutCurrent) {
        Buffer b(100);
        b.append("test", 4);

        void* mutableData = b.mutData();
        void* mutableCurrent = b.mutCurrent();

        STD_INSIST(mutableData != nullptr);
        STD_INSIST(mutableCurrent == advancePtr(mutableData, 4));
    }

    STD_TEST(length_equals_used) {
        Buffer b;
        STD_INSIST(b.length() == b.used());

        b.append("test", 4);
        STD_INSIST(b.length() == b.used());
        STD_INSIST(b.length() == 4);
    }

    STD_TEST(multiple_operations) {
        Buffer b(50);

        b.append("Part1", 5);
        STD_INSIST(b.used() == 5);

        b.append("Part2", 5);
        STD_INSIST(b.used() == 10);

        b.reset();
        STD_INSIST(b.empty());

        b.append("New", 3);
        STD_INSIST(b.used() == 3);
        STD_INSIST(memCmp(b.data(), "New", 3) == 0);
    }

    STD_TEST(advancePtr_helper) {
        int data[10];
        int* ptr = data;

        int* advanced = advancePtr(ptr, sizeof(int) * 5);
        STD_INSIST(advanced == &data[5]);

        const int* constPtr = data;
        const int* advancedConst = advancePtr(constPtr, sizeof(int) * 3);
        STD_INSIST(advancedConst == &data[3]);
    }

    STD_TEST(Test1) {
        Buffer l;
        Buffer r;
        l.append(r.data(), l.length());
        STD_INSIST(l.empty());
    }
}
