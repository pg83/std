#include "shared_future.h"
#include "coro.h"
#include "thread.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(SharedFuture) {
    STD_TEST(Basic) {
        auto f = makeIntrusivePtr(new SharedFutureBase<int>());

        f->post(42);
        STD_INSIST(f->wait() == 42);
    }

    STD_TEST(TwoThreads) {
        auto f = makeIntrusivePtr(new SharedFutureBase<int>());

        ScopedThread t([&] {
            f->post(123);
        });

        STD_INSIST(f->wait() == 123);
    }

    STD_TEST(Struct) {
        struct Point {
            int x, y;
        };

        auto f = makeIntrusivePtr(new SharedFutureBase<Point>());

        f->post(Point{3, 7});
        auto& p = f->wait();
        STD_INSIST(p.x == 3 && p.y == 7);
    }

    STD_TEST(CoroBasic) {
        auto exec = CoroExecutor::create(4);
        auto f = makeIntrusivePtr(new SharedFutureBase<int>(exec.mutPtr()));

        exec->spawn([&] {
            exec->spawn([&] {
                f->post(99);
            });

            STD_INSIST(f->wait() == 99);
        });

        exec->join();
    }

    STD_TEST(CoroMultiple) {
        const int N = 8;
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                auto f = makeIntrusivePtr(new SharedFutureBase<int>(exec.mutPtr()));

                exec->spawn([&, i] {
                    f->post(int(i));
                });

                STD_INSIST(f->wait() == i);
            }
        });

        exec->join();
    }

    STD_TEST(SharedOwnership) {
        auto f = makeIntrusivePtr(new SharedFutureBase<int>());
        auto f2 = f;

        ScopedThread t([f2]() mutable {
            f2->post(777);
        });

        STD_INSIST(f->wait() == 777);
    }
}
