#include "coro.h"
#include "latch.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(CoroExecutor) {
    STD_TEST(Basic) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool);
        int counter = 0;

        exec->spawn([](Cont* c, void* ctxx) {
            ++*(int*)ctxx;
        }, &counter);

        pool->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(SingleYield) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool);
        int counter = 0;

        exec->spawn([](Cont* c, void* ctxx) {
            ++*(int*)ctxx;
            c->executor()->yield();
            ++*(int*)ctxx;
        }, &counter);

        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(MultipleYields) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool);
        int counter = 0;

        exec->spawn([](Cont* c, void* ctxx) {
            for (int i = 0; i < 10; ++i) {
                ++*(int*)ctxx;
                c->executor()->yield();
            }
        }, &counter);

        pool->join();
        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyCoros) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([](Cont* c, void* ctxx) {
                c->executor()->yield();
                stdAtomicAddAndFetch((int*)ctxx, 1, MemoryOrder::Relaxed);
            }, &counter);
        }

        pool->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(NestedSpawn) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool);
        int counter = 0;

        struct Ctx {
            CoroExecutor* exec;
            int* counter;
        };
        Ctx ctxx{exec.mutPtr(), &counter};

        exec->spawn([](Cont* c, void* vctxx) {
            auto& ctxx = *(Ctx*)vctxx;

            ctxx.exec->spawn([](Cont*, void* vctxx2) {
                stdAtomicAddAndFetch((int*)((Ctx*)vctxx2)->counter, 1, MemoryOrder::Relaxed);
            }, &ctxx);

            stdAtomicAddAndFetch(ctxx.counter, 1, MemoryOrder::Relaxed);
        }, &ctxx);

        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(YieldInterleave) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool);
        int order = 0;
        Latch done(2);

        struct Ctx {
            CoroExecutor* exec;
            int* order;
            int id;
            Latch* done;
        };
        Ctx c1{exec.mutPtr(), &order, 1, &done};
        Ctx c2{exec.mutPtr(), &order, 2, &done};

        auto fn = [](Cont* c, void* vctxx) {
            auto& ctxx = *(Ctx*)vctxx;
            for (int i = 0; i < 5; ++i) {
                c->executor()->yield();
            }
            ctxx.done->arrive();
        };

        exec->spawn(fn, &c1);
        exec->spawn(fn, &c2);

        done.wait();
        pool->join();
    }
}
