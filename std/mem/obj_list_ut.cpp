#include "obj_list.h"

#include <std/tst/ut.h>
#include <std/sys/types.h>

using namespace stl;

namespace {
    struct SimpleStruct {
        int x;
        int y;

        SimpleStruct(int a, int b)
            : x(a)
            , y(b)
        {
        }
    };

    struct WithDestructor {
        int* counter;

        WithDestructor(int* c)
            : counter(c)
        {
            ++(*counter);
        }

        ~WithDestructor() {
            --(*counter);
        }
    };

    struct DefaultConstructible {
        int value = 42;
    };

    struct MultipleArgs {
        int a, b, c, d;

        MultipleArgs(int a_, int b_, int c_, int d_)
            : a(a_)
            , b(b_)
            , c(c_)
            , d(d_)
        {
        }
    };

    struct LargeObject {
        char data[1024];
        int id;

        LargeObject(int i)
            : id(i)
        {
            for (int j = 0; j < 1024; ++j) {
                data[j] = static_cast<char>(j & 0xFF);
            }
        }
    };

    struct LargeWithDestructor {
        char data[2048];
        int* counter;

        LargeWithDestructor(int* c)
            : counter(c)
        {
            ++(*counter);
        }

        ~LargeWithDestructor() {
            --(*counter);
        }
    };

    struct PODType {
        int x;
        double y;
        char z;
    };
}

STD_TEST_SUITE(ObjList) {
    STD_TEST(MakeReturnsNonNull) {
        ObjList<SimpleStruct> list;

        SimpleStruct* obj = list.make(10, 20);

        STD_INSIST(obj != nullptr);
    }

    STD_TEST(MakeInitializesValues) {
        ObjList<SimpleStruct> list;

        SimpleStruct* obj = list.make(10, 20);

        STD_INSIST(obj->x == 10);
        STD_INSIST(obj->y == 20);
    }

    STD_TEST(MakeMultipleObjects) {
        ObjList<SimpleStruct> list;

        SimpleStruct* obj1 = list.make(1, 2);
        SimpleStruct* obj2 = list.make(3, 4);
        SimpleStruct* obj3 = list.make(5, 6);

        STD_INSIST(obj1->x == 1);
        STD_INSIST(obj1->y == 2);
        STD_INSIST(obj2->x == 3);
        STD_INSIST(obj2->y == 4);
        STD_INSIST(obj3->x == 5);
        STD_INSIST(obj3->y == 6);
    }

    STD_TEST(MakeDifferentAddresses) {
        ObjList<SimpleStruct> list;

        SimpleStruct* obj1 = list.make(1, 2);
        SimpleStruct* obj2 = list.make(3, 4);

        STD_INSIST(obj1 != obj2);
    }

    STD_TEST(ReleaseObject) {
        ObjList<int> list;

        int* obj = list.make(42);
        STD_INSIST(*obj == 42);

        list.release(obj);
    }

    STD_TEST(ReleaseAndReallocate) {
        ObjList<int> list;

        int* obj1 = list.make(100);
        void* addr1 = obj1;

        list.release(obj1);

        int* obj2 = list.make(200);
        void* addr2 = obj2;

        STD_INSIST(addr1 == addr2);
        STD_INSIST(*obj2 == 200);
    }

    STD_TEST(ReleaseMultiple) {
        ObjList<SimpleStruct> list;

        SimpleStruct* obj1 = list.make(1, 2);
        SimpleStruct* obj2 = list.make(3, 4);
        SimpleStruct* obj3 = list.make(5, 6);

        list.release(obj1);
        list.release(obj2);
        list.release(obj3);
    }

    STD_TEST(ReleaseAndReallocateMultiple) {
        ObjList<int> list;

        int* obj1 = list.make(1);
        int* obj2 = list.make(2);
        int* obj3 = list.make(3);

        void* addr1 = obj1;
        void* addr2 = obj2;
        void* addr3 = obj3;

        list.release(obj1);
        list.release(obj2);
        list.release(obj3);

        int* obj4 = list.make(4);
        int* obj5 = list.make(5);
        int* obj6 = list.make(6);

        STD_INSIST(obj4 == addr3);
        STD_INSIST(obj5 == addr2);
        STD_INSIST(obj6 == addr1);
    }

    STD_TEST(DestructorCalled) {
        int counter = 0;

        {
            ObjList<WithDestructor> list;
            WithDestructor* obj = list.make(&counter);

            STD_INSIST(counter == 1);

            list.release(obj);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(DestructorCalledMultipleTimes) {
        int counter = 0;
        ObjList<WithDestructor> list;

        WithDestructor* obj1 = list.make(&counter);
        WithDestructor* obj2 = list.make(&counter);
        WithDestructor* obj3 = list.make(&counter);

        STD_INSIST(counter == 3);

        list.release(obj1);
        STD_INSIST(counter == 2);

        list.release(obj2);
        STD_INSIST(counter == 1);

        list.release(obj3);
        STD_INSIST(counter == 0);
    }

    STD_TEST(MakeDefaultConstructible) {
        ObjList<DefaultConstructible> list;

        DefaultConstructible* obj = list.make();

        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->value == 42);
    }

    STD_TEST(MakeMultipleArgs) {
        ObjList<MultipleArgs> list;

        MultipleArgs* obj = list.make(1, 2, 3, 4);

        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->a == 1);
        STD_INSIST(obj->b == 2);
        STD_INSIST(obj->c == 3);
        STD_INSIST(obj->d == 4);
    }

    STD_TEST(MakePODType) {
        ObjList<PODType> list;

        PODType* obj = list.make();

        STD_INSIST(obj != nullptr);
    }

    STD_TEST(MakePrimitiveInt) {
        ObjList<int> list;

        int* p = list.make(42);

        STD_INSIST(p != nullptr);
        STD_INSIST(*p == 42);
    }

    STD_TEST(MakePrimitiveDouble) {
        ObjList<double> list;

        double* p = list.make(3.14);

        STD_INSIST(p != nullptr);
        STD_INSIST(*p == 3.14);
    }

    STD_TEST(MakeLargeObject) {
        ObjList<LargeObject> list;

        LargeObject* obj = list.make(123);

        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->id == 123);
    }

    STD_TEST(MakeLargeObjectWithDestructor) {
        int counter = 0;
        ObjList<LargeWithDestructor> list;

        LargeWithDestructor* obj = list.make(&counter);

        STD_INSIST(obj != nullptr);
        STD_INSIST(counter == 1);

        list.release(obj);
        STD_INSIST(counter == 0);
    }

    STD_TEST(ObjectAlignment) {
        ObjList<SimpleStruct> list;

        SimpleStruct* obj = list.make(1, 2);

        STD_INSIST(reinterpret_cast<uintptr_t>(obj) % alignof(SimpleStruct) == 0);
    }

    STD_TEST(ManySmallAllocations) {
        ObjList<SimpleStruct> list;

        for (int i = 0; i < 1000; ++i) {
            SimpleStruct* obj = list.make(i, i * 2);
            STD_INSIST(obj->x == i);
            STD_INSIST(obj->y == i * 2);
        }
    }

    STD_TEST(ManyAllocationsAndReleases) {
        ObjList<int> list;
        constexpr int count = 100;
        int* objects[count];

        for (int i = 0; i < count; ++i) {
            objects[i] = list.make(i);
            STD_INSIST(*objects[i] == i);
        }

        for (int i = 0; i < count; ++i) {
            list.release(objects[i]);
        }

        for (int i = 0; i < count; ++i) {
            int* obj = list.make(i + 1000);
            STD_INSIST(*obj == i + 1000);
        }
    }

    STD_TEST(InterleavedAllocateAndRelease) {
        ObjList<int> list;

        int* obj1 = list.make(1);
        int* obj2 = list.make(2);

        list.release(obj1);

        int* obj3 = list.make(3);
        STD_INSIST(obj3 == obj1);
        STD_INSIST(*obj3 == 3);

        int* obj4 = list.make(4);

        list.release(obj2);
        list.release(obj3);

        int* obj5 = list.make(5);
        STD_INSIST(obj5 == obj3);

        int* obj6 = list.make(6);
        STD_INSIST(obj6 == obj2);
    }

    STD_TEST(MultipleLists) {
        ObjList<int> list1;
        ObjList<int> list2;

        int* obj1 = list1.make(10);
        int* obj2 = list2.make(20);

        STD_INSIST(*obj1 == 10);
        STD_INSIST(*obj2 == 20);

        list1.release(obj1);
        list2.release(obj2);
    }

    STD_TEST(DifferentTypes) {
        ObjList<int> intList;
        ObjList<double> doubleList;
        ObjList<SimpleStruct> structList;

        int* i = intList.make(42);
        double* d = doubleList.make(3.14);
        SimpleStruct* s = structList.make(1, 2);

        STD_INSIST(*i == 42);
        STD_INSIST(*d == 3.14);
        STD_INSIST(s->x == 1);
        STD_INSIST(s->y == 2);

        intList.release(i);
        doubleList.release(d);
        structList.release(s);
    }

    STD_TEST(ReuseAfterRelease) {
        int counter = 0;
        ObjList<WithDestructor> list;

        WithDestructor* obj1 = list.make(&counter);
        STD_INSIST(counter == 1);

        list.release(obj1);
        STD_INSIST(counter == 0);

        WithDestructor* obj2 = list.make(&counter);
        STD_INSIST(counter == 1);
        STD_INSIST(obj1 == obj2);

        list.release(obj2);
        STD_INSIST(counter == 0);
    }

    STD_TEST(StressTest) {
        ObjList<SimpleStruct> list;

        for (int i = 0; i < 10000; ++i) {
            SimpleStruct* obj = list.make(i, i * 2);
            STD_INSIST(obj->x == i);
            STD_INSIST(obj->y == i * 2);
        }
    }

    STD_TEST(StressTestWithReleases) {
        ObjList<int> list;
        constexpr int count = 1000;
        int* objects[count];

        for (int iteration = 0; iteration < 10; ++iteration) {
            for (int i = 0; i < count; ++i) {
                objects[i] = list.make(iteration * count + i);
            }

            for (int i = 0; i < count; ++i) {
                STD_INSIST(*objects[i] == iteration * count + i);
            }

            for (int i = 0; i < count; ++i) {
                list.release(objects[i]);
            }
        }
    }

    STD_TEST(StressTestWithDestructors) {
        int counter = 0;
        ObjList<WithDestructor> list;

        for (int i = 0; i < 1000; ++i) {
            WithDestructor* obj = list.make(&counter);
            STD_INSIST(counter == 1);
            list.release(obj);
            STD_INSIST(counter == 0);
        }
    }
}
