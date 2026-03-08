#include "coro.h"
#include "pool.h"
#include "latch.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

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
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
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
        pool->join();
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
}
