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
        Thread t1(w1);
        Thread t2(w2);

        wg.wait();

        t1.join();
        t2.join();

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

        Thread t0(workers[0]);
        Thread t1(workers[1]);
        Thread t2(workers[2]);
        Thread t3(workers[3]);
        Thread t4(workers[4]);
        Thread t5(workers[5]);
        Thread t6(workers[6]);
        Thread t7(workers[7]);

        wg.wait();

        t0.join(); t1.join(); t2.join(); t3.join();
        t4.join(); t5.join(); t6.join(); t7.join();

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
        Thread t1(w1);

        wg.add(1);
        Thread t2(w2);

        wg.add(1);
        Thread t3(w3);

        wg.wait();

        t1.join();
        t2.join();
        t3.join();

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
            Thread t(w);
            wg.wait();
            t.join();
        }

        STD_INSIST(counter == 3);
    }
}
