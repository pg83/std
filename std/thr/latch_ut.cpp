#include "latch.h"
#include "coro.h"
#include "pool.h"
#include "thread.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(Latch) {
    STD_TEST(SingleArrive) {
        Latch latch(1);
        latch.arrive();
        latch.wait();
    }

    STD_TEST(MultipleArrives) {
        Latch latch(3);
        latch.arrive();
        latch.arrive();
        latch.arrive();
        latch.wait();
    }

    STD_TEST(WaitAlreadyDone) {
        Latch latch(1);
        latch.arrive();
        latch.wait();
        latch.wait();
    }

    STD_TEST(TwoThreads) {
        Latch latch(2);
        int counter = 0;

        auto worker = [&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            latch.arrive();
        };

        ScopedThread t1(worker);
        ScopedThread t2(worker);

        latch.wait();
        STD_INSIST(counter == 2);
    }

    STD_TEST(ManyThreads) {
        const int N = 8;
        Latch latch(N);
        int counter = 0;

        auto worker = [&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            latch.arrive();
        };

        ScopedThread t0(worker);
        ScopedThread t1(worker);
        ScopedThread t2(worker);
        ScopedThread t3(worker);
        ScopedThread t4(worker);
        ScopedThread t5(worker);
        ScopedThread t6(worker);
        ScopedThread t7(worker);

        latch.wait();
        STD_INSIST(counter == N);
    }

    STD_TEST(CoroBasic) {
        auto exec = CoroExecutor::create(4);
        Latch latch(1, exec.mutPtr());
        int counter = 0;

        exec->spawn([&](Cont*) {
            latch.wait();
            STD_INSIST(counter == 1);
        });

        exec->spawn([&](Cont*) {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            latch.arrive();
        });

        exec->join();
    }

    STD_TEST(CoroMultiple) {
        const int N = 8;
        auto exec = CoroExecutor::create(4);
        Latch latch(N, exec.mutPtr());
        int counter = 0;

        exec->spawn([&](Cont*) {
            latch.wait();
            STD_INSIST(counter == N);
        });

        for (int i = 0; i < N; ++i) {
            exec->spawn([&](Cont*) {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                latch.arrive();
            });
        }

        exec->join();
    }

    STD_TEST(CoroManyWaiters) {
        const int N = 4;
        auto exec = CoroExecutor::create(4);
        Latch latch(1, exec.mutPtr());
        int counter = 0;

        for (int i = 0; i < N; ++i) {
            exec->spawn([&](Cont*) {
                latch.wait();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        exec->spawn([&](Cont*) {
            latch.arrive();
        });

        exec->join();
        STD_INSIST(counter == N);
    }
}
