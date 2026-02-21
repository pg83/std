#include "obj_allocator.h"
#include "mem_pool.h"

#include <std/tst/ut.h>
#include <std/sys/types.h>

using namespace Std;

namespace {
    struct TestStruct {
        int a;
        int b;
        char c;
    };

    struct LargeStruct {
        char data[1024];
        int id;
    };
}

STD_TEST_SUITE(ObjAllocator) {
    STD_TEST(AllocateReturnsNonNull) {
        MemoryPool pool;
        ObjAllocator allocator(64, &pool);

        void* ptr = allocator.allocate();

        STD_INSIST(ptr != nullptr);
    }

    STD_TEST(AllocateReturnsDifferentAddresses) {
        MemoryPool pool;
        ObjAllocator allocator(32, &pool);

        void* ptr1 = allocator.allocate();
        void* ptr2 = allocator.allocate();

        STD_INSIST(ptr1 != nullptr);
        STD_INSIST(ptr2 != nullptr);
        STD_INSIST(ptr1 != ptr2);
    }

    STD_TEST(ReleaseNullPointerDoesNotCrash) {
        MemoryPool pool;
        ObjAllocator allocator(64, &pool);

        allocator.release(nullptr);

        STD_INSIST(true);
    }

    STD_TEST(AllocateAfterRelease) {
        MemoryPool pool;
        ObjAllocator allocator(64, &pool);

        void* ptr1 = allocator.allocate();
        allocator.release(ptr1);
        void* ptr2 = allocator.allocate();

        STD_INSIST(ptr1 == ptr2);
    }

    STD_TEST(AllocateAfterMultipleReleases) {
        MemoryPool pool;
        ObjAllocator allocator(64, &pool);

        void* ptr1 = allocator.allocate();
        void* ptr2 = allocator.allocate();
        void* ptr3 = allocator.allocate();

        allocator.release(ptr1);
        allocator.release(ptr2);
        allocator.release(ptr3);

        void* ptr4 = allocator.allocate();
        void* ptr5 = allocator.allocate();
        void* ptr6 = allocator.allocate();

        STD_INSIST(ptr4 == ptr3);
        STD_INSIST(ptr5 == ptr2);
        STD_INSIST(ptr6 == ptr1);
    }

    STD_TEST(AllocateAndReleasePattern) {
        MemoryPool pool;
        ObjAllocator allocator(sizeof(int), &pool);

        void* ptr1 = allocator.allocate();
        void* ptr2 = allocator.allocate();

        allocator.release(ptr1);

        void* ptr3 = allocator.allocate();
        STD_INSIST(ptr3 == ptr1);

        void* ptr4 = allocator.allocate();
        STD_INSIST(ptr4 != ptr1);
        STD_INSIST(ptr4 != ptr2);
        STD_INSIST(ptr4 != ptr3);
    }

    STD_TEST(WorksWithSmallObjectSize) {
        MemoryPool pool;
        ObjAllocator allocator(1, &pool);

        void* ptr1 = allocator.allocate();
        void* ptr2 = allocator.allocate();

        STD_INSIST(ptr1 != nullptr);
        STD_INSIST(ptr2 != nullptr);
        STD_INSIST(ptr1 != ptr2);

        allocator.release(ptr1);
        void* ptr3 = allocator.allocate();
        STD_INSIST(ptr3 == ptr1);
    }

    STD_TEST(WorksWithLargeObjects) {
        MemoryPool pool;
        ObjAllocator allocator(sizeof(LargeStruct), &pool);

        LargeStruct* obj1 = static_cast<LargeStruct*>(allocator.allocate());
        LargeStruct* obj2 = static_cast<LargeStruct*>(allocator.allocate());

        obj1->id = 42;
        obj2->id = 100;

        STD_INSIST(obj1->id == 42);
        STD_INSIST(obj2->id == 100);

        allocator.release(obj1);
        allocator.release(obj2);

        LargeStruct* obj3 = static_cast<LargeStruct*>(allocator.allocate());
        STD_INSIST(obj3 == obj2);
    }

    STD_TEST(AllocateAndAccessManyObjects) {
        MemoryPool pool;
        ObjAllocator allocator(sizeof(TestStruct), &pool);

        constexpr size_t count = 100;
        TestStruct* objects[count];

        for (size_t i = 0; i < count; ++i) {
            objects[i] = static_cast<TestStruct*>(allocator.allocate());
            objects[i]->a = static_cast<int>(i);
            objects[i]->b = static_cast<int>(i * 2);
            objects[i]->c = 'A' + (i % 26);
        }

        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(objects[i]->a == static_cast<int>(i));
            STD_INSIST(objects[i]->b == static_cast<int>(i * 2));
            STD_INSIST(objects[i]->c == 'A' + (i % 26));
        }
    }

    STD_TEST(ReleaseAndReallocateMany) {
        MemoryPool pool;
        ObjAllocator allocator(sizeof(int), &pool);

        constexpr size_t count = 50;
        int* pointers[count];

        for (size_t i = 0; i < count; ++i) {
            pointers[i] = static_cast<int*>(allocator.allocate());
        }

        for (size_t i = 0; i < count; ++i) {
            allocator.release(pointers[i]);
        }

        for (size_t i = 0; i < count; ++i) {
            int* ptr = static_cast<int*>(allocator.allocate());
            STD_INSIST(ptr == pointers[count - 1 - i]);
        }
    }

    STD_TEST(InterleavedAllocateAndRelease) {
        MemoryPool pool;
        ObjAllocator allocator(sizeof(double), &pool);

        double* ptr1 = static_cast<double*>(allocator.allocate());
        double* ptr2 = static_cast<double*>(allocator.allocate());
        
        allocator.release(ptr1);
        
        double* ptr3 = static_cast<double*>(allocator.allocate());
        STD_INSIST(ptr3 == ptr1);
        
        double* ptr4 = static_cast<double*>(allocator.allocate());
        
        allocator.release(ptr2);
        allocator.release(ptr3);
        
        double* ptr5 = static_cast<double*>(allocator.allocate());
        STD_INSIST(ptr5 == ptr3);
        
        double* ptr6 = static_cast<double*>(allocator.allocate());
        STD_INSIST(ptr6 == ptr2);
    }

    STD_TEST(MultipleAllocatorsOnSamePool) {
        MemoryPool pool;
        ObjAllocator allocator1(32, &pool);
        ObjAllocator allocator2(64, &pool);

        void* ptr1 = allocator1.allocate();
        void* ptr2 = allocator2.allocate();

        STD_INSIST(ptr1 != nullptr);
        STD_INSIST(ptr2 != nullptr);

        allocator1.release(ptr1);
        allocator2.release(ptr2);

        void* ptr3 = allocator1.allocate();
        void* ptr4 = allocator2.allocate();

        STD_INSIST(ptr3 == ptr1);
        STD_INSIST(ptr4 == ptr2);
    }
}
