#include "wait_group.h"
#include "coro.h"
#include "latch.h"
#include "pool.h"
#include "thread.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(WaitGroup) {
    STD_TEST(SingleThread) {
        WaitGroup wg;
        int counter = 0;

        wg.add(1);
        ++counter;
        wg.done();

        wg.wait();
        STD_INSIST(counter == 1);
    }

    STD_TEST(TwoThreads) {
        WaitGroup wg;
        int counter = 0;

        wg.add(2);
        ScopedThread t1([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t2([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        wg.wait();
        STD_INSIST(counter == 2);
    }

    STD_TEST(ManyThreads) {
        const int N = 8;
        WaitGroup wg;
        int counter = 0;

        wg.add(N);
        ScopedThread t0([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t1([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t2([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t3([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t4([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t5([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t6([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });
        ScopedThread t7([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        wg.wait();
        STD_INSIST(counter == N);
    }

    STD_TEST(AddMultipleTimes) {
        WaitGroup wg;
        int counter = 0;

        wg.add(1);
        ScopedThread t1([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        wg.add(1);
        ScopedThread t2([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        wg.add(1);
        ScopedThread t3([&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            wg.done();
        });

        wg.wait();
        STD_INSIST(counter == 3);
    }

    STD_TEST(ReuseAfterWait) {
        WaitGroup wg;
        int counter = 0;

        for (int round = 0; round < 3; ++round) {
            wg.add(1);
            ScopedThread t([&] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                wg.done();
            });
            wg.wait();
        }

        STD_INSIST(counter == 3);
    }

    STD_TEST(CoroBasic) {
        auto exec = CoroExecutor::create(4);
        Latch done(1);
        WaitGroup wg(exec.mutPtr());
        int counter = 0;

        exec->spawn([&](Cont*) {
            wg.add(1);

            exec->spawn([&](Cont*) {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                wg.done();
            });

            wg.wait();
            STD_INSIST(counter == 1);
            done.arrive();
        });

        done.wait();
        exec->pool()->join();
    }

    STD_TEST(CoroMultiple) {
        const int N = 8;
        auto exec = CoroExecutor::create(4);
        Latch done(1);
        WaitGroup wg(exec.mutPtr());
        int counter = 0;

        exec->spawn([&](Cont*) {
            wg.add(N);

            for (int i = 0; i < N; ++i) {
                exec->spawn([&](Cont*) {
                    stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                    wg.done();
                });
            }

            wg.wait();
            STD_INSIST(counter == N);
            done.arrive();
        });

        done.wait();
        exec->pool()->join();
    }

    STD_TEST(CoroReuseAfterWait) {
        auto exec = CoroExecutor::create(4);
        Latch done(1);
        WaitGroup wg(exec.mutPtr());
        int counter = 0;

        exec->spawn([&](Cont*) {
            for (int round = 0; round < 3; ++round) {
                wg.add(1);

                exec->spawn([&](Cont*) {
                    stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                    wg.done();
                });

                wg.wait();
            }

            STD_INSIST(counter == 3);
            done.arrive();
        });

        done.wait();
        exec->pool()->join();
    }
}
