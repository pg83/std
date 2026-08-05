#include "disposer.h"

#include <std/tst/ut.h>
#include <std/mem/new.h>
#include <std/typ/support.h>

using namespace stl;

namespace {
    struct TestArena {
        alignas(max_align_t) u8 storage[4096];
        size_t used = 0;

        template <typename T, typename... A>
        T* make(A&&... a) {
            size_t offset = (used + alignof(T) - 1) & ~(alignof(T) - 1);

            STD_INSIST(offset + sizeof(T) <= sizeof(storage));
            used = offset + sizeof(T);

            return new (storage + offset) T(forward<A>(a)...);
        }
    };

    struct TestDisposable: public Disposable, public Newable {
        int* counter;

        TestDisposable(int* c)
            : counter(c)
        {
            ++(*counter);
        }

        ~TestDisposable() noexcept override {
            --(*counter);
        }
    };

    struct OrderTracker: public Disposable, public Newable {
        int id;
        int* lastDestroyed;

        OrderTracker(int i, int* ld)
            : id(i)
            , lastDestroyed(ld)
        {
        }

        ~OrderTracker() noexcept override {
            *lastDestroyed = id;
        }
    };

    struct SimpleDisposable: public Disposable, public Newable {
        int value;

        SimpleDisposable(int v)
            : value(v)
        {
        }

        ~SimpleDisposable() noexcept override = default;
    };
}

STD_TEST_SUITE(Disposer) {
    STD_TEST(empty_disposer) {
        Disposer disposer;
        STD_INSIST(disposer.length() == 0);
    }

    STD_TEST(submit_single) {
        int counter = 0;
        TestArena arena;
        auto obj = arena.make<TestDisposable>(&counter);

        {
            Disposer disposer;
            disposer.submit(obj);

            STD_INSIST(disposer.length() == 1);
            STD_INSIST(counter == 1);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(submit_multiple) {
        int counter = 0;
        TestArena arena;

        {
            Disposer disposer;
            disposer.submit(arena.make<TestDisposable>(&counter));
            disposer.submit(arena.make<TestDisposable>(&counter));
            disposer.submit(arena.make<TestDisposable>(&counter));

            STD_INSIST(disposer.length() == 3);
            STD_INSIST(counter == 3);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(dispose_destroys_objects) {
        int counter = 0;
        TestArena arena;

        Disposer disposer;
        disposer.submit(arena.make<TestDisposable>(&counter));
        disposer.submit(arena.make<TestDisposable>(&counter));

        STD_INSIST(counter == 2);
        STD_INSIST(disposer.length() == 2);

        disposer.dispose();

        STD_INSIST(counter == 0);
        STD_INSIST(disposer.length() == 0);
    }

    STD_TEST(dispose_called_multiple_times) {
        int counter = 0;
        TestArena arena;

        Disposer disposer;
        disposer.submit(arena.make<TestDisposable>(&counter));

        STD_INSIST(counter == 1);

        disposer.dispose();
        STD_INSIST(counter == 0);

        disposer.dispose();
        STD_INSIST(counter == 0);
    }

    STD_TEST(destructor_calls_dispose) {
        int counter = 0;
        TestArena arena;

        {
            Disposer disposer;
            disposer.submit(arena.make<TestDisposable>(&counter));
            STD_INSIST(counter == 1);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(disposal_order_lifo) {
        int lastDestroyed = 0;
        TestArena arena;

        {
            Disposer disposer;
            disposer.submit(arena.make<OrderTracker>(1, &lastDestroyed));
            disposer.submit(arena.make<OrderTracker>(2, &lastDestroyed));
            disposer.submit(arena.make<OrderTracker>(3, &lastDestroyed));
        }

        STD_INSIST(lastDestroyed == 1);
    }

    STD_TEST(length_after_dispose) {
        TestArena arena;
        Disposer disposer;
        disposer.submit(arena.make<SimpleDisposable>(1));
        disposer.submit(arena.make<SimpleDisposable>(2));

        STD_INSIST(disposer.length() == 2);

        disposer.dispose();

        STD_INSIST(disposer.length() == 0);
    }

    STD_TEST(submit_after_dispose) {
        int counter = 0;
        TestArena arena;

        {
            Disposer disposer;
            disposer.submit(arena.make<TestDisposable>(&counter));

            STD_INSIST(counter == 1);

            disposer.dispose();
            STD_INSIST(counter == 0);

            disposer.submit(arena.make<TestDisposable>(&counter));
            STD_INSIST(counter == 1);
            STD_INSIST(disposer.length() == 1);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(many_objects) {
        int counter = 0;
        TestArena arena;

        {
            Disposer disposer;

            for (int i = 0; i < 100; ++i) {
                disposer.submit(arena.make<TestDisposable>(&counter));
            }

            STD_INSIST(counter == 100);
            STD_INSIST(disposer.length() == 100);
        }

        STD_INSIST(counter == 0);
    }

    STD_TEST(empty_dispose) {
        Disposer disposer;
        disposer.dispose();

        STD_INSIST(disposer.length() == 0);
    }

    STD_TEST(mixed_types) {
        int counter1 = 0;
        int counter2 = 0;
        TestArena arena;

        struct OtherDisposable: public Disposable, public Newable {
            int* counter;

            OtherDisposable(int* c)
                : counter(c)
            {
                ++(*counter);
            }

            ~OtherDisposable() noexcept override {
                --(*counter);
            }
        };

        {
            Disposer disposer;
            disposer.submit(arena.make<TestDisposable>(&counter1));
            disposer.submit(arena.make<OtherDisposable>(&counter2));
            disposer.submit(arena.make<TestDisposable>(&counter1));

            STD_INSIST(counter1 == 2);
            STD_INSIST(counter2 == 1);
        }

        STD_INSIST(counter1 == 0);
        STD_INSIST(counter2 == 0);
    }
}
