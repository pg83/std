#include "obj_pool.h"

#include <std/tst/ut.h>
#include <std/str/view.h>

#include <cstddef>

using namespace Std;

namespace {
    struct SimpleStruct {
        int x;
        int y;

        SimpleStruct(int a, int b) : x(a), y(b) {}
    };

    struct WithDestructor {
        int* counter;

        WithDestructor(int* c) : counter(c) {
            ++(*counter);
        }

        ~WithDestructor() {
            --(*counter);
        }
    };

    struct ComplexObject {
        int value;
        StringView name;
        int* destructorCounter;

        ComplexObject(int v, StringView n, int* c)
            : value(v), name(n), destructorCounter(c)
        {
            ++(*destructorCounter);
        }

        ~ComplexObject() {
            --(*destructorCounter);
        }
    };

    struct DefaultConstructible {
        int value = 42;
    };

    struct MultipleArgs {
        int a, b, c, d;

        MultipleArgs(int a_, int b_, int c_, int d_)
            : a(a_), b(b_), c(c_), d(d_) {}
    };

    struct LargeObject {
        char data[1024];
        int id;

        LargeObject(int i) : id(i) {
            for (int j = 0; j < 1024; ++j) {
                data[j] = static_cast<char>(j & 0xFF);
            }
        }
    };

    struct LargeWithDestructor {
        char data[2048];
        int* counter;

        LargeWithDestructor(int* c) : counter(c) {
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

STD_TEST_SUITE(ObjPool) {
    STD_TEST(allocate_returns_non_null) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* ptr = pool->allocate(64);

        STD_INSIST(ptr != nullptr);
    }

    STD_TEST(allocate_different_addresses) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* ptr1 = pool->allocate(32);
        void* ptr2 = pool->allocate(32);

        STD_INSIST(ptr1 != ptr2);
    }

    STD_TEST(allocate_aligned) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* ptr = pool->allocate(1);

        STD_INSIST(reinterpret_cast<uintptr_t>(ptr) % alignof(std::max_align_t) == 0);
    }

    STD_TEST(allocate_multiple_aligned) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        for (int i = 0; i < 100; ++i) {
            void* ptr = pool->allocate(i + 1);
            STD_INSIST(reinterpret_cast<uintptr_t>(ptr) % alignof(std::max_align_t) == 0);
        }
    }

    STD_TEST(allocate_large_block) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* ptr = pool->allocate(4096);

        STD_INSIST(ptr != nullptr);
    }

    STD_TEST(allocate_very_large_block) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* ptr = pool->allocate(1024 * 1024);

        STD_INSIST(ptr != nullptr);
    }

    STD_TEST(allocate_zero_size) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* ptr = pool->allocate(0);

        STD_INSIST(ptr != nullptr);
    }

    STD_TEST(make_pod_type) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        PODType* obj = pool->make<PODType>();

        STD_INSIST(obj != nullptr);
    }

    STD_TEST(make_simple_struct) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        SimpleStruct* obj = pool->make<SimpleStruct>(10, 20);


        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->x == 10);
        STD_INSIST(obj->y == 20);
    }

    STD_TEST(make_default_constructible) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        DefaultConstructible* obj = pool->make<DefaultConstructible>();

        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->value == 42);
    }

    STD_TEST(make_multiple_args) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        MultipleArgs* obj = pool->make<MultipleArgs>(1, 2, 3, 4);

        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->a == 1);
        STD_INSIST(obj->b == 2);
        STD_INSIST(obj->c == 3);
        STD_INSIST(obj->d == 4);
    }

    STD_TEST(make_with_destructor) {
        int counter = 0;

        {
            ObjPool::Ref pool = ObjPool::fromMemory();
            WithDestructor* obj = pool->make<WithDestructor>(&counter);

            STD_INSIST(obj != nullptr);
            STD_INSIST(counter == 1);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(make_multiple_with_destructor) {
        int counter = 0;

        {
            ObjPool::Ref pool = ObjPool::fromMemory();

            pool->make<WithDestructor>(&counter);
            pool->make<WithDestructor>(&counter);
            pool->make<WithDestructor>(&counter);

            STD_INSIST(counter == 3);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(make_complex_object) {
        int counter = 0;

        {
            ObjPool::Ref pool = ObjPool::fromMemory();
            StringView name = pool->intern(StringView("test"));

            ComplexObject* obj = pool->make<ComplexObject>(42, name, &counter);

            STD_INSIST(obj != nullptr);
            STD_INSIST(obj->value == 42);
            STD_INSIST(obj->name == StringView("test"));
            STD_INSIST(counter == 1);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(make_large_object_no_destructor) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        LargeObject* obj = pool->make<LargeObject>(123);

        STD_INSIST(obj != nullptr);
        STD_INSIST(obj->id == 123);
    }

    STD_TEST(make_large_object_with_destructor) {
        int counter = 0;

        {
            ObjPool::Ref pool = ObjPool::fromMemory();
            LargeWithDestructor* obj = pool->make<LargeWithDestructor>(&counter);

            STD_INSIST(obj != nullptr);
            STD_INSIST(counter == 1);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(intern_empty_string) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        StringView result = pool->intern(StringView(""));

        STD_INSIST(result.length() == 0);
    }

    STD_TEST(intern_simple_string) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        StringView result = pool->intern(StringView("hello"));

        STD_INSIST(result == StringView("hello"));
        STD_INSIST(result.length() == 5);
    }

    STD_TEST(intern_preserves_content) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        char original[] = "test string";
        StringView result = pool->intern(StringView((const char*)original));
        original[0] = 'X';
        STD_INSIST(result == StringView("test string"));
    }

    STD_TEST(intern_different_addresses) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        StringView s1 = pool->intern(StringView("first"));
        StringView s2 = pool->intern(StringView("second"));

        STD_INSIST(s1.data() != s2.data());
    }

    STD_TEST(intern_long_string) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        const char* longStr = "this is a very long string that should still work correctly with the pool allocator";
        StringView result = pool->intern(StringView(longStr));

        STD_INSIST(result == StringView(longStr));
    }

    STD_TEST(intern_multiple_strings) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        StringView s1 = pool->intern(StringView("one"));
        StringView s2 = pool->intern(StringView("two"));
        StringView s3 = pool->intern(StringView("three"));

        STD_INSIST(s1 == StringView("one"));
        STD_INSIST(s2 == StringView("two"));
        STD_INSIST(s3 == StringView("three"));
    }

    STD_TEST(mixed_allocations) {
        int counter = 0;

        {
            ObjPool::Ref pool = ObjPool::fromMemory();

            StringView s = pool->intern(StringView("test"));
            SimpleStruct* simple = pool->make<SimpleStruct>(1, 2);
            WithDestructor* wd = pool->make<WithDestructor>(&counter);
            void* raw = pool->allocate(64);

            STD_INSIST(s == StringView("test"));
            STD_INSIST(simple->x == 1);
            STD_INSIST(counter == 1);
            STD_INSIST(raw != nullptr);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(many_small_allocations) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        for (int i = 0; i < 1000; ++i) {
            SimpleStruct* obj = pool->make<SimpleStruct>(i, i * 2);
            STD_INSIST(obj->x == i);
            STD_INSIST(obj->y == i * 2);
        }
    }

    STD_TEST(many_destructible_objects) {
        int counter = 0;

        {
            ObjPool::Ref pool = ObjPool::fromMemory();

            for (int i = 0; i < 100; ++i) {
                pool->make<WithDestructor>(&counter);
            }

            STD_INSIST(counter == 100);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(trigger_new_chunk_allocation) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        for (int i = 0; i < 100; ++i) {
            void* ptr = pool->allocate(256);
            STD_INSIST(ptr != nullptr);
        }
    }

    STD_TEST(large_allocation_triggers_chunk) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        void* small = pool->allocate(10);
        void* large = pool->allocate(1024);

        STD_INSIST(small != nullptr);
        STD_INSIST(large != nullptr);
    }

    STD_TEST(ref_counting) {
        ObjPool::Ref pool1 = ObjPool::fromMemory();

        {
            ObjPool::Ref pool2 = pool1;
            void* ptr = pool2->allocate(32);
            STD_INSIST(ptr != nullptr);
        }

        void* ptr = pool1->allocate(32);
        STD_INSIST(ptr != nullptr);
    }

    STD_TEST(destructor_order_reverse) {
        int lastDestroyed = 0;

        struct OrderChecker {
            int id;
            int* lastDestroyed;

            OrderChecker(int i, int* ld) : id(i), lastDestroyed(ld) {}

            ~OrderChecker() {
                *lastDestroyed = id;
            }
        };

        {
            ObjPool::Ref pool = ObjPool::fromMemory();

            pool->make<OrderChecker>(1, &lastDestroyed);
            pool->make<OrderChecker>(2, &lastDestroyed);
            pool->make<OrderChecker>(3, &lastDestroyed);
        }

        STD_INSIST(lastDestroyed == 1);
    }

    STD_TEST(make_primitive_int) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        int* p = pool->make<int>(42);

        STD_INSIST(p != nullptr);
        STD_INSIST(*p == 42);
    }

    STD_TEST(make_primitive_double) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        double* p = pool->make<double>(3.14);

        STD_INSIST(p != nullptr);
        STD_INSIST(*p == 3.14);
    }

    STD_TEST(object_alignment) {
        ObjPool::Ref pool = ObjPool::fromMemory();

        struct Aligned {
            double d;
            int i;
        };

        Aligned* obj = pool->make<Aligned>();

        STD_INSIST(reinterpret_cast<uintptr_t>(obj) % alignof(Aligned) == 0);
    }

    STD_TEST(stress) {
        auto pool = ObjPool::fromMemory();

        for (size_t i = 0; i < 1000; ++i) {
            pool->allocate(i);
        }
    }

    STD_TEST(obj) {
        struct T {
            u64* ptr;

            inline T(u64* _ptr) noexcept
                : ptr(_ptr)
            {
            }

            inline ~T() {
                *ptr = 0;
            }
        };

        u64 v = 1;

        {
            auto pool = ObjPool::fromMemory();
            auto obj = pool->make<T>(&v);

            STD_INSIST(v == 1);
            STD_INSIST(obj->ptr == &v);
            STD_INSIST(*obj->ptr == 1);
        }

        STD_INSIST(v == 0);
    }

    STD_TEST(testPrimitive) {
        ObjPool::fromMemory()->make<double>();
    }
}
