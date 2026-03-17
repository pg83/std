#include "future.h"
#include "coro.h"
#include "thread.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(Future) {
    STD_TEST(Basic) {
        Future f;

        f.complete((void*)42);
        STD_INSIST(f.wait() == (void*)42);
    }

    STD_TEST(TwoThreads) {
        Future f;

        ScopedThread t([&] {
            f.complete((void*)123);
        });

        STD_INSIST(f.wait() == (void*)123);
    }

    STD_TEST(NullValue) {
        Future f;

        f.complete(nullptr);
        STD_INSIST(f.wait() == nullptr);
    }

    STD_TEST(CoroBasic) {
        auto exec = CoroExecutor::create(4);
        Future f(exec.mutPtr());

        exec->spawn([&] {
            exec->spawn([&] {
                f.complete((void*)99);
            });

            STD_INSIST(f.wait() == (void*)99);
        });

        exec->join();
    }

    STD_TEST(CoroMultiple) {
        const int N = 8;
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                Future f(exec.mutPtr());

                exec->spawn([&] {
                    f.complete((void*)(size_t)i);
                });

                STD_INSIST(f.wait() == (void*)(size_t)i);
            }
        });

        exec->join();
    }
}
