#include "coro.h"
#include "pool.h"
#include "latch.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <functional>

using namespace stl;

namespace {
    static void doW(int work) {
        for (volatile int i = 0; i < work; ++i) {
        }
    }
}

STD_TEST_SUITE(CoroExecutor) {
    STD_TEST(Basic) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont*) {
            ++counter;
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(SingleYield) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont* c) {
            ++counter;
            c->executor()->yield();
            ++counter;
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(MultipleYields) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont* c) {
            for (int i = 0; i < 10; ++i) {
                ++counter;
                c->executor()->yield();
            }
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyCoros) {
        auto exec = CoroExecutor::create(4);
        Latch done(100);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&](Cont* c) {
                c->executor()->yield();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                done.arrive();
            });
        }

        done.wait();
        exec->pool()->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(NestedSpawn) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(2);
        int counter = 0;

        exec->spawn([&](Cont*) {
            exec->spawn([&](Cont*) {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                done.arrive();
            });

            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(YieldInterleave) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(2);

        auto fn = [&](Cont* c) {
            for (int i = 0; i < 5; ++i) {
                c->executor()->yield();
            }
            done.arrive();
        };

        exec->spawn(fn);
        exec->spawn(fn);

        done.wait();
        pool->join();
    }

    const int depth = 22;
    const int work = 999;

    STD_TEST(_SW) {
        auto exec = CoroExecutor::create(16);

        int counter = 1;
        int counter2 = 0;
        Mutex mutex;
        CondVar condVar;

        std::function<void(Cont*, int)> run = [&](Cont* c, int d) {
            stdAtomicAddAndFetch(&counter2, 1, MemoryOrder::Relaxed);

            doW(work);

            if (d > 0) {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);

                exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d]() {
                    run(exec->me(), d - 1);
                }));
            }

            c->executor()->yield();

            if (d > 0) {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);

                exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d]() {
                    run(exec->me(), d - 1);
                }));
            }

            doW(work);

            if (stdAtomicSubAndFetch(&counter, 1, MemoryOrder::Release) == 0) {
                LockGuard lock(mutex);
                condVar.signal();
            }
        };

        exec->spawn([&](Cont* c) {
            run(c, depth);
        });

        {
            LockGuard lock(mutex);

            while (stdAtomicFetch(&counter, MemoryOrder::Acquire) > 0) {
                condVar.wait(mutex);
            }
        }

        exec->pool()->join();

        _ctx.output() << counter2 << endL << flsH;
    }
}
