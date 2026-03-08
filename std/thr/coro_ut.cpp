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

        struct Ctx {
            int* counter;
            Latch* done;
        };
        Ctx ctx{&counter, &done};

        exec->spawnCoro([](Cont* c, void* v) {
            auto& ctx = *(Ctx*)v;
            ++*ctx.counter;
            ctx.done->arrive();
        }, &ctx);

        done.wait();
        pool->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(SingleYield) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        struct Ctx {
            int* counter;
            Latch* done;
        };
        Ctx ctx{&counter, &done};

        exec->spawnCoro([](Cont* c, void* v) {
            auto& ctx = *(Ctx*)v;
            ++*ctx.counter;
            c->executor()->yield();
            ++*ctx.counter;
            ctx.done->arrive();
        }, &ctx);

        done.wait();
        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(MultipleYields) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        struct Ctx {
            int* counter;
            Latch* done;
        };
        Ctx ctx{&counter, &done};

        exec->spawnCoro([](Cont* c, void* v) {
            auto& ctx = *(Ctx*)v;
            for (int i = 0; i < 10; ++i) {
                ++*ctx.counter;
                c->executor()->yield();
            }
            ctx.done->arrive();
        }, &ctx);

        done.wait();
        pool->join();
        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyCoros) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(100);
        int counter = 0;

        struct Ctx {
            int* counter;
            Latch* done;
        };
        Ctx ctx{&counter, &done};

        for (int i = 0; i < 100; ++i) {
            exec->spawnCoro([](Cont* c, void* v) {
                auto& ctx = *(Ctx*)v;
                c->executor()->yield();
                stdAtomicAddAndFetch(ctx.counter, 1, MemoryOrder::Relaxed);
                ctx.done->arrive();
            }, &ctx);
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

        struct Ctx {
            CoroExecutor* exec;
            int* counter;
            Latch* done;
        };
        Ctx ctx{exec.mutPtr(), &counter, &done};

        exec->spawnCoro([](Cont* c, void* v) {
            auto& ctx = *(Ctx*)v;

            ctx.exec->spawnCoro([](Cont*, void* v2) {
                auto& ctx2 = *(Ctx*)v2;
                stdAtomicAddAndFetch(ctx2.counter, 1, MemoryOrder::Relaxed);
                ctx2.done->arrive();
            }, &ctx);

            stdAtomicAddAndFetch(ctx.counter, 1, MemoryOrder::Relaxed);
            ctx.done->arrive();
        }, &ctx);

        done.wait();
        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(YieldInterleave) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(2);

        struct Ctx {
            Latch* done;
        };
        Ctx ctx{&done};

        auto fn = [](Cont* c, void* v) {
            auto& ctx = *(Ctx*)v;
            for (int i = 0; i < 5; ++i) {
                c->executor()->yield();
            }
            ctx.done->arrive();
        };

        exec->spawnCoro(fn, &ctx);
        exec->spawnCoro(fn, &ctx);

        done.wait();
        pool->join();
    }
}
