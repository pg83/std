#include "thread_pool.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace Std;

namespace {
    struct CounterTask: public Task {
        int* counter_;

        explicit CounterTask(int* c)
            : counter_(c)
        {
        }

        void run() noexcept override {
            ++(*counter_);
        }
    };

    struct AtomicCounterTask: public Task {
        int* counter_;

        explicit AtomicCounterTask(int* c)
            : counter_(c)
        {
        }

        void run() noexcept override {
            stdAtomicAddAndFetch(counter_, 1, MemoryOrder::Relaxed);
            delete this;
        }
    };

    struct SleepTask: public Task {
        void run() noexcept override {
            for (volatile int i = 0; i < 10000; ++i) {
            }
        }
    };
}

STD_TEST_SUITE(ThreadPool) {
    STD_TEST(BasicConstruction) {
        auto pool = ThreadPool::simple(4);
        pool->join();
    }

    STD_TEST(SingleTask) {
        auto pool = ThreadPool::simple(1);
        int counter = 0;

        CounterTask task(&counter);
        pool->submit(task);
        pool->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleTasks) {
        auto pool = ThreadPool::simple(2);
        int counter = 0;

        CounterTask task1(&counter);
        CounterTask task2(&counter);
        CounterTask task3(&counter);

        pool->submit(task1);
        pool->submit(task2);
        pool->submit(task3);
        pool->join();

        STD_INSIST(counter == 3);
    }

    STD_TEST(ManyTasksSingleThread) {
        auto pool = ThreadPool::simple(1);
        int counter = 0;
        const int numTasks = 100;

        CounterTask tasks[numTasks] = {
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter),
        };

        for (int i = 0; i < numTasks; ++i) {
            pool->submit(tasks[i]);
        }
        pool->join();

        STD_INSIST(counter == numTasks);
    }

    STD_TEST(ManyTasksMultipleThreads) {
        auto pool = ThreadPool::simple(4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(EmptyPool) {
        auto pool = ThreadPool::simple(2);
        pool->join();
    }

    STD_TEST(TasksWithWork) {
        auto pool = ThreadPool::simple(4);

        SleepTask tasks[20] = {
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
        };

        for (int i = 0; i < 20; ++i) {
            pool->submit(tasks[i]);
        }
        pool->join();
    }

    STD_TEST(SingleThreadPool) {
        auto pool = ThreadPool::simple(1);
        int counter = 0;

        for (int i = 0; i < 10; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyThreadsPool) {
        auto pool = ThreadPool::simple(8);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 100);
    }
}
