#include "latch.h"
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
}
