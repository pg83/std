#include "future.h"
#include "coro.h"
#include "thread.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(Future) {
    STD_TEST(Basic) {
        Future f;

        f.post((void*)42);
        STD_INSIST(f.wait() == (void*)42);
    }

    STD_TEST(TwoThreads) {
        Future f;

        ScopedThread t([&] {
            f.post((void*)123);
        });

        STD_INSIST(f.wait() == (void*)123);
    }

    STD_TEST(NullValue) {
        Future f;

        f.post(nullptr);
        STD_INSIST(f.wait() == nullptr);
    }

    STD_TEST(CoroBasic) {
        auto exec = CoroExecutor::create(4);
        Future f(exec.mutPtr());

        exec->spawn([&] {
            exec->spawn([&] {
                f.post((void*)99);
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
                    f.post((void*)(size_t)i);
                });

                STD_INSIST(f.wait() == (void*)(size_t)i);
            }
        });

        exec->join();
    }

    STD_TEST(CoroRecursive) {
        auto exec = CoroExecutor::create(4);
        Future result;

        auto run = [&](auto& self, size_t depth, Future& out) -> void {
            if (depth == 0) {
                return out.post((void*)(size_t)1);
            }

            Future f1(exec.mutPtr());
            Future f2(exec.mutPtr());

            exec->spawn([&, depth]() {
                self(self, depth - 1, f1);
            });

            exec->spawn([&, depth]() {
                self(self, depth - 1, f2);
            });

            auto v = (size_t)f1.wait() + (size_t)f2.wait();

            out.post((void*)v);
        };

        exec->spawn([&] {
            run(run, 10, result);
        });

        STD_INSIST((size_t)result.wait() == 1024);

        exec->join();
    }
}
