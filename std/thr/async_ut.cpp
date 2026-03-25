#include "coro.h"
#include "pool.h"
#include "async.h"

#include <std/tst/ut.h>
#include <std/mem/obj_pool.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(ThreadAsync) {
    STD_TEST(Basic) {
        auto f = async([] {
            return 42;
        });

        STD_INSIST(f.wait() == 42);
    }

    STD_TEST(Struct) {
        struct Point {
            int x, y;
        };

        auto f = async([] {
            return Point{3, 7};
        });

        auto& p = f.wait();
        STD_INSIST(p.x == 3 && p.y == 7);
    }

    STD_TEST(Parallel) {
        auto f1 = async([] {
            return 1;
        });
        auto f2 = async([] {
            return 2;
        });
        auto f3 = async([] {
            return 3;
        });
        auto f4 = async([] {
            return 4;
        });

        int sum = f1.wait() + f2.wait() + f3.wait() + f4.wait();
        STD_INSIST(sum == 10);
    }
}

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

        auto f1 = async(pool, [] {
            return 1;
        });
        auto f2 = async(pool, [] {
            return 2;
        });
        auto f3 = async(pool, [] {
            return 3;
        });
        auto f4 = async(pool, [] {
            return 4;
        });

        int sum = f1.wait() + f2.wait() + f3.wait() + f4.wait();
        STD_INSIST(sum == 10);
        pool->join();
    }
}

namespace {
    struct Tracked {
        int value;
        int* alive;

        Tracked(int v, int* a) noexcept
            : value(v)
            , alive(a)
        {
            stdAtomicAddAndFetch(alive, 1, MemoryOrder::Relaxed);
        }

        Tracked(const Tracked& o) noexcept
            : value(o.value)
            , alive(o.alive)
        {
            stdAtomicAddAndFetch(alive, 1, MemoryOrder::Relaxed);
        }

        ~Tracked() noexcept {
            stdAtomicSubAndFetch(alive, 1, MemoryOrder::Relaxed);
        }
    };
}

STD_TEST_SUITE(AsyncLifetime) {
    STD_TEST(Pool) {
        int alive = 0;
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        {
            auto f = async(pool, [&] {
                return Tracked(42, &alive);
            });

            STD_INSIST(f.wait().value == 42);
        }

        pool->join();
        STD_INSIST(stdAtomicFetch(&alive, MemoryOrder::Acquire) == 0);
    }

    STD_TEST(PoolMultiple) {
        int alive = 0;
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        {
            auto f1 = async(pool, [&] {
                return Tracked(1, &alive);
            });
            auto f2 = async(pool, [&] {
                return Tracked(2, &alive);
            });
            auto f3 = async(pool, [&] {
                return Tracked(3, &alive);
            });

            STD_INSIST(f1.wait().value == 1);
            STD_INSIST(f2.wait().value == 2);
            STD_INSIST(f3.wait().value == 3);
        }

        pool->join();
        STD_INSIST(stdAtomicFetch(&alive, MemoryOrder::Acquire) == 0);
    }

    STD_TEST(PoolLoop) {
        int alive = 0;
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        for (int i = 0; i < 10; ++i) {
            auto f = async(pool, [&, i] {
                return Tracked(i, &alive);
            });

            STD_INSIST(f.wait().value == i);
        }

        pool->join();
        STD_INSIST(stdAtomicFetch(&alive, MemoryOrder::Acquire) == 0);
    }

    STD_TEST(Coro) {
        int alive = 0;
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            auto f1 = async(exec.mutPtr(), [&] {
                return Tracked(10, &alive);
            });
            auto f2 = async(exec.mutPtr(), [&] {
                return Tracked(20, &alive);
            });

            STD_INSIST(f1.wait().value == 10);
            STD_INSIST(f2.wait().value == 20);
        });

        exec->join();
        STD_INSIST(stdAtomicFetch(&alive, MemoryOrder::Acquire) == 0);
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
            auto f1 = async(exec.mutPtr(), [] {
                return 1;
            });
            auto f2 = async(exec.mutPtr(), [] {
                return 2;
            });
            auto f3 = async(exec.mutPtr(), [] {
                return 3;
            });
            auto f4 = async(exec.mutPtr(), [] {
                return 4;
            });

            int sum = f1.wait() + f2.wait() + f3.wait() + f4.wait();
            STD_INSIST(sum == 10);
        });

        exec->join();
    }

    STD_TEST(Parallel2) {
        auto exec = CoroExecutor::create(4);

        auto res = async(exec.mutPtr(), [&] {
                       auto f1 = async(exec.mutPtr(), [] {
                           return 1;
                       });
                       auto f2 = async(exec.mutPtr(), [] {
                           return 2;
                       });
                       auto f3 = async(exec.mutPtr(), [] {
                           return 3;
                       });
                       auto f4 = async(exec.mutPtr(), [] {
                           return 4;
                       });

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

            auto f1 = async(exec.mutPtr(), [&] {
                return self(self, depth - 1);
            });

            auto f2 = async(exec.mutPtr(), [&] {
                return self(self, depth - 1);
            });

            return f1.wait() + f2.wait();
        };

        auto f = async(exec.mutPtr(), [&] {
            return run(run, 8);
        });

        STD_INSIST(f.wait() == 256);
    }
}
