#include "barrier.h"
#include "coro.h"
#include "pool.h"
#include "thread.h"
#include "runable.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

namespace {
    struct WaitRunable: public Runable {
        Barrier* barrier;
        int* counter;

        WaitRunable(Barrier* b, int* c)
            : barrier(b)
            , counter(c)
        {
        }

        void run() noexcept override {
            barrier->wait();
            stdAtomicAddAndFetch(counter, 1, MemoryOrder::Relaxed);
        }
    };

    struct OrderRunable: public Runable {
        Barrier* barrier;
        int* arrived;
        int total;
        bool* correct;

        OrderRunable(Barrier* b, int* a, int n, bool* c)
            : barrier(b)
            , arrived(a)
            , total(n)
            , correct(c)
        {
        }

        void run() noexcept override {
            stdAtomicAddAndFetch(arrived, 1, MemoryOrder::Release);
            barrier->wait();
            if (stdAtomicFetch(arrived, MemoryOrder::Acquire) != total) {
                *correct = false;
            }
        }
    };
}

STD_TEST_SUITE(Barrier) {
    STD_TEST(BasicConstruction) {
        Barrier b1(1);
        Barrier b2(2);
        Barrier b3(10);
    }

    STD_TEST(SingleParticipant) {
        Barrier barrier(1);
        barrier.wait();
    }

    STD_TEST(TwoThreads) {
        Barrier barrier(2);
        int counter = 0;

        WaitRunable r1(&barrier, &counter);
        WaitRunable r2(&barrier, &counter);

        Thread t1(r1);
        Thread t2(r2);

        t1.join();
        t2.join();

        STD_INSIST(counter == 2);
    }

    STD_TEST(ManyThreads) {
        const int N = 8;
        Barrier barrier(N);
        int counter = 0;

        WaitRunable r1(&barrier, &counter);
        WaitRunable r2(&barrier, &counter);
        WaitRunable r3(&barrier, &counter);
        WaitRunable r4(&barrier, &counter);
        WaitRunable r5(&barrier, &counter);
        WaitRunable r6(&barrier, &counter);
        WaitRunable r7(&barrier, &counter);
        WaitRunable r8(&barrier, &counter);

        Thread t1(r1);
        Thread t2(r2);
        Thread t3(r3);
        Thread t4(r4);
        Thread t5(r5);
        Thread t6(r6);
        Thread t7(r7);
        Thread t8(r8);

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();
        t6.join();
        t7.join();
        t8.join();

        STD_INSIST(counter == N);
    }

    // Проверяем, что ни один поток не проходит барьер до тех пор,
    // пока все не вызвали wait()
    STD_TEST(NonePassBeforeAll) {
        const int N = 4;
        Barrier barrier(N);
        int arrived = 0;
        bool correct = true;

        OrderRunable r1(&barrier, &arrived, N, &correct);
        OrderRunable r2(&barrier, &arrived, N, &correct);
        OrderRunable r3(&barrier, &arrived, N, &correct);
        OrderRunable r4(&barrier, &arrived, N, &correct);

        Thread t1(r1);
        Thread t2(r2);
        Thread t3(r3);
        Thread t4(r4);

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        STD_INSIST(correct);
    }

    STD_TEST(CoroBasic) {
        const int N = 6;
        auto exec = CoroExecutor::create(4);
        Barrier barrier(N, exec.mutPtr());
        int arrived = 0;
        bool correct = true;

        for (int i = 0; i < N; ++i) {
            exec->spawn([&]() {
                stdAtomicAddAndFetch(&arrived, 1, MemoryOrder::Release);
                barrier.wait();
                if (stdAtomicFetch(&arrived, MemoryOrder::Acquire) != N) {
                    correct = false;
                }
            });
        }

        exec->join();
        STD_INSIST(correct);
    }

    STD_TEST(CoroMultipleBarriers) {
        const int N = 4;
        auto exec = CoroExecutor::create(4);
        Barrier b1(N, exec.mutPtr());
        Barrier b2(N, exec.mutPtr());
        int phase = 0;

        for (int i = 0; i < N; ++i) {
            exec->spawn([&]() {
                b1.wait();
                stdAtomicAddAndFetch(&phase, 1, MemoryOrder::Relaxed);
                b2.wait();
                stdAtomicAddAndFetch(&phase, 1, MemoryOrder::Relaxed);
            });
        }

        exec->join();
        STD_INSIST(phase == N * 2);
    }
}
