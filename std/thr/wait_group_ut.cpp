#include "coro.h"
#include "pool.h"
#include "thread.h"
#include "runable.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/alg/defer.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(WaitGroup) {
    STD_TEST(SingleThread) {
        auto pool = ObjPool::fromMemory();
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0);
        int counter = 0;

        wg.add(1);
        ++counter;
        wg.done();

        wg.wait();
        STD_INSIST(counter == 1);
    }

    STD_TEST(TwoThreads) {
        auto pool = ObjPool::fromMemory();
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0);
        int counter = 0;

        wg.add(2);

        auto r1 = makeRunable([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        auto* t1 = Thread::create(pool.mutPtr(), r1);
        STD_DEFER { t1->join(); };

        auto r2 = makeRunable([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        auto* t2 = Thread::create(pool.mutPtr(), r2);
        STD_DEFER { t2->join(); };

        wg.wait();
        STD_INSIST(counter == 2);
    }

    STD_TEST(ManyThreads) {
        const int N = 8;
        auto pool = ObjPool::fromMemory();
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0);
        int counter = 0;

        wg.add(N);

        auto work = [&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        };

        auto r0 = makeRunable(work);
        auto r1 = makeRunable(work);
        auto r2 = makeRunable(work);
        auto r3 = makeRunable(work);
        auto r4 = makeRunable(work);
        auto r5 = makeRunable(work);
        auto r6 = makeRunable(work);
        auto r7 = makeRunable(work);

        auto* t0 = Thread::create(pool.mutPtr(), r0);
        STD_DEFER { t0->join(); };
        auto* t1 = Thread::create(pool.mutPtr(), r1);
        STD_DEFER { t1->join(); };
        auto* t2 = Thread::create(pool.mutPtr(), r2);
        STD_DEFER { t2->join(); };
        auto* t3 = Thread::create(pool.mutPtr(), r3);
        STD_DEFER { t3->join(); };
        auto* t4 = Thread::create(pool.mutPtr(), r4);
        STD_DEFER { t4->join(); };
        auto* t5 = Thread::create(pool.mutPtr(), r5);
        STD_DEFER { t5->join(); };
        auto* t6 = Thread::create(pool.mutPtr(), r6);
        STD_DEFER { t6->join(); };
        auto* t7 = Thread::create(pool.mutPtr(), r7);
        STD_DEFER { t7->join(); };

        wg.wait();
        STD_INSIST(counter == N);
    }

    STD_TEST(AddMultipleTimes) {
        auto pool = ObjPool::fromMemory();
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0);
        int counter = 0;

        wg.add(1);

        auto r1 = makeRunable([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        auto* t1 = Thread::create(pool.mutPtr(), r1);
        STD_DEFER { t1->join(); };

        wg.add(1);

        auto r2 = makeRunable([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        auto* t2 = Thread::create(pool.mutPtr(), r2);
        STD_DEFER { t2->join(); };

        wg.add(1);

        auto r3 = makeRunable([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        auto* t3 = Thread::create(pool.mutPtr(), r3);
        STD_DEFER { t3->join(); };

        wg.wait();
        STD_INSIST(counter == 3);
    }

    STD_TEST(ReuseAfterWait) {
        auto pool = ObjPool::fromMemory();
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0);
        int counter = 0;

        for (int round = 0; round < 3; ++round) {
            wg.add(1);

            auto r = makeRunable([&] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                wg.done();
            });

            auto* t = Thread::create(pool.mutPtr(), r);
            STD_DEFER { t->join(); };

            wg.wait();
        }

        STD_INSIST(counter == 3);
    }

    STD_TEST(CoroBasic) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);
        int counter = 0;

        exec->spawn([&] {
            wg.add(1);

            exec->spawn([&] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                wg.done();
            });

            wg.wait();
            STD_INSIST(counter == 1);
        });

        exec->join();
    }

    STD_TEST(CoroMultiple) {
        const int N = 8;
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);
        int counter = 0;

        exec->spawn([&] {
            wg.add(N);

            for (int i = 0; i < N; ++i) {
                exec->spawn([&] {
                    stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                    wg.done();
                });
            }

            wg.wait();
            STD_INSIST(counter == N);
        });

        exec->join();
    }

    STD_TEST(CoroReuseAfterWait) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);
        int counter = 0;

        exec->spawn([&] {
            for (int round = 0; round < 3; ++round) {
                wg.add(1);

                exec->spawn([&] {
                    stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                    wg.done();
                });

                wg.wait();
            }

            STD_INSIST(counter == 3);
        });

        exec->join();
    }
}
