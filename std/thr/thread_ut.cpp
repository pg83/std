#include "thread.h"

#include <std/tst/ut.h>

using namespace Std;

namespace {
    struct CounterRunnable: public Runnable {
        int* counter;

        explicit CounterRunnable(int* c)
            : counter(c)
        {
        }

        void run() override {
            ++(*counter);
        }
    };

    struct MultiIncrementRunnable: public Runnable {
        int* counter;
        int iterations;

        MultiIncrementRunnable(int* c, int iter)
            : counter(c)
            , iterations(iter)
        {
        }

        void run() override {
            for (int i = 0; i < iterations; ++i) {
                ++(*counter);
            }
        }
    };
}

STD_TEST_SUITE(Thread) {
    STD_TEST(BasicThreadCreationAndJoin) {
        int counter = 0;
        CounterRunnable runnable(&counter);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleThreads) {
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        CounterRunnable runnable1(&counter1);
        CounterRunnable runnable2(&counter2);
        CounterRunnable runnable3(&counter3);

        Thread thread1(runnable1);
        Thread thread2(runnable2);
        Thread thread3(runnable3);

        thread1.join();
        thread2.join();
        thread3.join();

        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
        STD_INSIST(counter3 == 1);
    }

    STD_TEST(ThreadWithMultipleIncrements) {
        int counter = 0;
        MultiIncrementRunnable runnable(&counter, 1000);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(counter == 1000);
    }

    STD_TEST(SequentialThreadCreation) {
        for (int i = 0; i < 10; ++i) {
            int counter = 0;
            CounterRunnable runnable(&counter);

            Thread thread(runnable);
            thread.join();

            STD_INSIST(counter == 1);
        }
    }

    STD_TEST(MultipleThreadsWithDifferentWorkloads) {
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        MultiIncrementRunnable runnable1(&counter1, 100);
        MultiIncrementRunnable runnable2(&counter2, 500);
        MultiIncrementRunnable runnable3(&counter3, 1000);

        Thread thread1(runnable1);
        Thread thread2(runnable2);
        Thread thread3(runnable3);

        thread1.join();
        thread2.join();
        thread3.join();

        STD_INSIST(counter1 == 100);
        STD_INSIST(counter2 == 500);
        STD_INSIST(counter3 == 1000);
    }

    STD_TEST(ThreadJoinOrder) {
        int counter1 = 0;
        int counter2 = 0;

        CounterRunnable runnable1(&counter1);
        CounterRunnable runnable2(&counter2);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        thread2.join();
        thread1.join();

        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
    }

    STD_TEST(EmptyRunnableExecution) {
        struct EmptyRunnable: public Runnable {
            bool* executed;

            explicit EmptyRunnable(bool* e)
                : executed(e)
            {
            }

            void run() override {
                *executed = true;
            }
        };

        bool executed = false;
        EmptyRunnable runnable(&executed);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(executed == true);
    }
}
