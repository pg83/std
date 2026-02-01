#include "thread.h"

#include <std/tst/ut.h>

using namespace Std;

namespace {
    struct CounterRunable: public Runable {
        int* counter;

        explicit CounterRunable(int* c)
            : counter(c)
        {
        }

        void run() override {
            ++(*counter);
        }
    };

    struct MultiIncrementRunable: public Runable {
        int* counter;
        int iterations;

        MultiIncrementRunable(int* c, int iter)
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
        CounterRunable runnable(&counter);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleThreads) {
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        CounterRunable runnable1(&counter1);
        CounterRunable runnable2(&counter2);
        CounterRunable runnable3(&counter3);

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
        MultiIncrementRunable runnable(&counter, 1000);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(counter == 1000);
    }

    STD_TEST(SequentialThreadCreation) {
        for (int i = 0; i < 10; ++i) {
            int counter = 0;
            CounterRunable runnable(&counter);

            Thread thread(runnable);
            thread.join();

            STD_INSIST(counter == 1);
        }
    }

    STD_TEST(MultipleThreadsWithDifferentWorkloads) {
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        MultiIncrementRunable runnable1(&counter1, 100);
        MultiIncrementRunable runnable2(&counter2, 500);
        MultiIncrementRunable runnable3(&counter3, 1000);

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

        CounterRunable runnable1(&counter1);
        CounterRunable runnable2(&counter2);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        thread2.join();
        thread1.join();

        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
    }

    STD_TEST(EmptyRunableExecution) {
        struct EmptyRunable: public Runable {
            bool* executed;

            explicit EmptyRunable(bool* e)
                : executed(e)
            {
            }

            void run() override {
                *executed = true;
            }
        };

        bool executed = false;
        EmptyRunable runnable(&executed);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(executed == true);
    }
}
