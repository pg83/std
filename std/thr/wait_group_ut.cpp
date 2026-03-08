#include "wait_group.h"
#include "thread.h"
#include "runable.h"

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

        struct Worker: public Runable {
            WaitGroup* wg;
            int* counter;

            Worker(WaitGroup* w, int* c)
                : wg(w)
                , counter(c)
            {
            }

            void run() noexcept override {
                stdAtomicAddAndFetch(counter, 1, MemoryOrder::Relaxed);
                wg->done();
            }
        };

        Worker w1(&wg, &counter);
        Worker w2(&wg, &counter);

        wg.add(2);
        ScopedThread t1(w1);
        ScopedThread t2(w2);

        wg.wait();

        STD_INSIST(counter == 2);
    }

    STD_TEST(ManyThreads) {
        const int N = 8;
        WaitGroup wg;
        int counter = 0;

        struct Worker: public Runable {
            WaitGroup* wg;
            int* counter;

            Worker(WaitGroup* w, int* c)
                : wg(w)
                , counter(c)
            {
            }

            void run() noexcept override {
                stdAtomicAddAndFetch(counter, 1, MemoryOrder::Relaxed);
                wg->done();
            }
        };

        Worker workers[N] = {
            {&wg, &counter}, {&wg, &counter}, {&wg, &counter}, {&wg, &counter},
            {&wg, &counter}, {&wg, &counter}, {&wg, &counter}, {&wg, &counter},
        };

        wg.add(N);

        ScopedThread t0(workers[0]);
        ScopedThread t1(workers[1]);
        ScopedThread t2(workers[2]);
        ScopedThread t3(workers[3]);
        ScopedThread t4(workers[4]);
        ScopedThread t5(workers[5]);
        ScopedThread t6(workers[6]);
        ScopedThread t7(workers[7]);

        wg.wait();

        STD_INSIST(counter == N);
    }

    STD_TEST(AddMultipleTimes) {
        WaitGroup wg;
        int counter = 0;

        struct Worker: public Runable {
            WaitGroup* wg;
            int* counter;

            Worker(WaitGroup* w, int* c)
                : wg(w)
                , counter(c)
            {
            }

            void run() noexcept override {
                stdAtomicAddAndFetch(counter, 1, MemoryOrder::Relaxed);
                wg->done();
            }
        };

        Worker w1(&wg, &counter);
        Worker w2(&wg, &counter);
        Worker w3(&wg, &counter);

        wg.add(1);
        ScopedThread t1(w1);

        wg.add(1);
        ScopedThread t2(w2);

        wg.add(1);
        ScopedThread t3(w3);

        wg.wait();

        STD_INSIST(counter == 3);
    }

    STD_TEST(ReuseAfterWait) {
        WaitGroup wg;
        int counter = 0;

        struct Worker: public Runable {
            WaitGroup* wg;
            int* counter;

            Worker(WaitGroup* w, int* c)
                : wg(w)
                , counter(c)
            {
            }

            void run() noexcept override {
                stdAtomicAddAndFetch(counter, 1, MemoryOrder::Relaxed);
                wg->done();
            }
        };

        for (int round = 0; round < 3; ++round) {
            Worker w(&wg, &counter);
            wg.add(1);
            ScopedThread t(w);
            wg.wait();
        }

        STD_INSIST(counter == 3);
    }
}
