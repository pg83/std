#include "mem_pool.h"

#include <std/tst/ut.h>
#include <std/sys/types.h>

#include <cstddef>

using namespace Std;

namespace {
    // Helper function to check if pointer is aligned
    bool isAligned(void* ptr, size_t alignment) {
        return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
    }
    
    // Simple struct for testing allocations
    struct TestStruct {
        int a;
        int b;
        char c;
    };
    
    // Larger struct for testing bigger allocations
    struct LargeStruct {
        char data[1024];
        int id;
    };
}

STD_TEST_SUITE(MemoryPool) {
    STD_TEST(allocate_returns_non_null) {
        MemoryPool pool;
        
        void* ptr = pool.allocate(64);
        
        STD_INSIST(ptr != nullptr);
    }
    
    STD_TEST(allocate_returns_aligned_pointers) {
        MemoryPool pool;
        
        void* ptr1 = pool.allocate(1);
        void* ptr2 = pool.allocate(16);
        void* ptr3 = pool.allocate(32);
        
        constexpr size_t alignment = alignof(std::max_align_t);
        STD_INSIST(isAligned(ptr1, alignment));
        STD_INSIST(isAligned(ptr2, alignment));
        STD_INSIST(isAligned(ptr3, alignment));
    }
    
    STD_TEST(allocate_different_sizes) {
        MemoryPool pool;
        
        void* ptr1 = pool.allocate(1);
        void* ptr2 = pool.allocate(8);
        void* ptr3 = pool.allocate(64);
        void* ptr4 = pool.allocate(1024);
        
        STD_INSIST(ptr1 != nullptr);
        STD_INSIST(ptr2 != nullptr);
        STD_INSIST(ptr3 != nullptr);
        STD_INSIST(ptr4 != nullptr);
    }
    
    STD_TEST(allocate_returns_different_addresses) {
        MemoryPool pool;
        
        void* ptr1 = pool.allocate(32);
        void* ptr2 = pool.allocate(32);
        
        STD_INSIST(ptr1 != ptr2);
    }
    
    STD_TEST(allocate_zero_size) {
        MemoryPool pool;
        
        void* ptr = pool.allocate(0);
        
        STD_INSIST(ptr != nullptr);
    }
    
    STD_TEST(allocate_multiple_then_access) {
        MemoryPool pool;
        
        int* intPtr = static_cast<int*>(pool.allocate(sizeof(int)));
        *intPtr = 42;
        
        double* doublePtr = static_cast<double*>(pool.allocate(sizeof(double)));
        *doublePtr = 3.14;
        
        TestStruct* structPtr = static_cast<TestStruct*>(pool.allocate(sizeof(TestStruct)));
        structPtr->a = 1;
        structPtr->b = 2;
        structPtr->c = 'x';
        
        STD_INSIST(*intPtr == 42);
        STD_INSIST(*doublePtr == 3.14);
        STD_INSIST(structPtr->a == 1);
        STD_INSIST(structPtr->b == 2);
        STD_INSIST(structPtr->c == 'x');
    }
    
    STD_TEST(allocate_large_chunks) {
        MemoryPool pool;
        
        // Allocate a large chunk to trigger new chunk allocation
        void* ptr1 = pool.allocate(1024);
        STD_INSIST(ptr1 != nullptr);
        
        // Allocate another large chunk
        void* ptr2 = pool.allocate(2048);
        STD_INSIST(ptr2 != nullptr);
        
        STD_INSIST(ptr1 != ptr2);
    }
    
    STD_TEST(allocate_many_small_objects) {
        MemoryPool pool;
        
        // Allocate many small objects
        constexpr size_t count = 100;
        int* pointers[count];
        
        for (size_t i = 0; i < count; ++i) {
            pointers[i] = static_cast<int*>(pool.allocate(sizeof(int)));
            *pointers[i] = static_cast<int>(i);
        }
        
        // Verify all values are correct
        for (size_t i = 0; i < count; ++i) {
            STD_INSIST(*pointers[i] == static_cast<int>(i));
        }
    }
    
    STD_TEST(pool_destructor_runs_without_error) {
        // This test verifies that the pool can be destroyed without issues
        {
            MemoryPool pool;
            [[maybe_unused]] void* ptr = pool.allocate(32);
            // Pool destructor will be called here
        }
        // If we reach here, destructor worked correctly
        STD_INSIST(true);
    }
}