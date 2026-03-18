#include "coro.h"
#include "pool.h"
#include "async.h"

#include <std/tst/ut.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(PoolAsync) {
    STD_TEST(Basic) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        auto f = async(pool, [] {
            return 42;
        });

        STD_INSIST(f.wait() == 42);
        pool->join();
    }

    STD_TEST(Struct) {
        struct Point {
            int x, y;
        };
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        auto f = async(pool, [] {
            return Point{3, 7};
        });

        auto& p = f.wait();
        STD_INSIST(p.x == 3 && p.y == 7);
        pool->join();
    }

    STD_TEST(Multiple) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);
        const int N = 8;

        for (int i = 0; i < N; ++i) {
            auto f = async(pool, [i] {
                return i * i;
            });

            STD_INSIST(f.wait() == i * i);
        }

        pool->join();
    }

    STD_TEST(Parallel) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        auto f1 = async(pool, [] { return 1; });
        auto f2 = async(pool, [] { return 2; });
        auto f3 = async(pool, [] { return 3; });
        auto f4 = async(pool, [] { return 4; });

        int sum = f1.wait() + f2.wait() + f3.wait() + f4.wait();
        STD_INSIST(sum == 10);
        pool->join();
    }
}

STD_TEST_SUITE(Async) {
    STD_TEST(Basic) {
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            auto f = async(exec.mutPtr(), [] {
                return 42;
            });

            STD_INSIST(f.wait() == 42);
        });

        exec->join();
    }

    STD_TEST(Struct) {
        struct Point {
            int x, y;
        };
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            auto f = async(exec.mutPtr(), [] {
                return Point{3, 7};
            });

            auto& p = f.wait();
            STD_INSIST(p.x == 3 && p.y == 7);
        });

        exec->join();
    }

    STD_TEST(Multiple) {
        auto exec = CoroExecutor::create(4);
        const int N = 8;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                auto f = async(exec.mutPtr(), [i] {
                    return i * i;
                });

                STD_INSIST(f.wait() == i * i);
            }
        });

        exec->join();
    }

    STD_TEST(Parallel) {
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            auto f1 = async(exec.mutPtr(), [] { return 1; });
            auto f2 = async(exec.mutPtr(), [] { return 2; });
            auto f3 = async(exec.mutPtr(), [] { return 3; });
            auto f4 = async(exec.mutPtr(), [] { return 4; });

            int sum = f1.wait() + f2.wait() + f3.wait() + f4.wait();
            STD_INSIST(sum == 10);
        });

        exec->join();
    }

    STD_TEST(Parallel2) {
        auto exec = CoroExecutor::create(4);

        auto res = async(exec.mutPtr(), [&] {
                       auto f1 = async(exec.mutPtr(), [] { return 1; });
                       auto f2 = async(exec.mutPtr(), [] { return 2; });
                       auto f3 = async(exec.mutPtr(), [] { return 3; });
                       auto f4 = async(exec.mutPtr(), [] { return 4; });

                       return f1.wait() + f2.wait() + f3.wait() + f4.wait();
                   }).wait();

        STD_INSIST(res == 10);
    }

    STD_TEST(CoroRecursive) {
        auto exec = CoroExecutor::create(4);

        auto run = [&](auto& self, size_t depth) {
            if (depth == 0) {
                return 1;
            }

            auto f1 = async(exec.mutPtr(), [&]() {
                return self(self, depth - 1);
            });

            auto f2 = async(exec.mutPtr(), [&]() {
                return self(self, depth - 1);
            });

            return f1.wait() + f2.wait();
        };

        auto f = async(exec.mutPtr(), [&]() {
            return run(run, 10);
        });

        STD_INSIST(f.wait() == 1024);
    }
}
