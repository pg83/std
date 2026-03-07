#include "pool.h"
#include "task.h"
#include "runable.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

namespace {
    static inline void doW(int work) {
        for (volatile int i = 0; i < work; ++i) {
        }
    }

    struct StressState {
        ThreadPool* pool;
        int work;
        int counter;
        Mutex mutex;
        CondVar condVar;

        StressState(ThreadPool* p, int w)
            : pool(p)
            , work(w)
            , counter(1)
        {
        }
    };

    struct StressTask: public Runable {
        StressState* state_;
        int depth_;

        StressTask(StressState* s, int d)
            : state_(s)
            , depth_(d)
        {
        }

        void doWork() noexcept {
            doW(state_->work);
        }

        void schedule() {
            stdAtomicAddAndFetch(&state_->counter, 1, MemoryOrder::Relaxed);
            state_->pool->submit(*new StressTask(state_, depth_ - 1));
        }

        void run() override {
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();

            auto* state = state_;
            delete this;

            if (stdAtomicSubAndFetch(&state->counter, 1, MemoryOrder::Release) == 0) {
                LockGuard lock(state->mutex);
                state->condVar.signal();
            }
        }
    };

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
        auto pool = ThreadPool::simple(1);
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
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
            CounterTask(&counter),
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
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
            SleepTask(),
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

STD_TEST_SUITE(WorkStealingThreadPool) {
    STD_TEST(BasicConstruction) {
        auto pool = ThreadPool::workStealing(4);
        pool->join();
    }

    STD_TEST(SingleTask) {
        auto pool = ThreadPool::workStealing(1);
        int counter = 0;

        CounterTask task(&counter);
        pool->submit(task);
        pool->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleTasks) {
        auto pool = ThreadPool::workStealing(2);
        int counter = 0;

        for (int i = 0; i < 10; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyTasksSingleThread) {
        auto pool = ThreadPool::workStealing(1);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(ManyTasksMultipleThreads) {
        auto pool = ThreadPool::workStealing(4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(EmptyPool) {
        auto pool = ThreadPool::workStealing(2);
        pool->join();
    }

    STD_TEST(TasksWithWork) {
        auto pool = ThreadPool::workStealing(4);
        int counter = 0;

        for (int i = 0; i < 20; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }

        for (volatile int i = 0; i < 10000; ++i) {
        }

        pool->join();

        STD_INSIST(counter == 20);
    }

    STD_TEST(SingleThreadPool) {
        auto pool = ThreadPool::workStealing(1);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 50);
    }

    STD_TEST(ManyThreadsPool) {
        auto pool = ThreadPool::workStealing(8);
        int counter = 0;

        for (int i = 0; i < 200; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 200);
    }

    STD_TEST(WorkStealing) {
        auto pool = ThreadPool::workStealing(4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(MixedWorkload) {
        auto pool = ThreadPool::workStealing(4);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }

        for (volatile int i = 0; i < 5000; ++i) {
        }

        for (int i = 0; i < 50; ++i) {
            pool->submit(*new AtomicCounterTask(&counter));
        }

        pool->join();

        STD_INSIST(counter == 100);
    }

    const int depth = 23;
    const int work = 1000;

    STD_TEST(_SW) {
        auto pool = ThreadPool::workStealing(16);
        StressState state(pool.mutPtr(), work);

        pool->submit(*new StressTask(&state, depth));

        {
            LockGuard lock(state.mutex);

            while (stdAtomicFetch(&state.counter, MemoryOrder::Acquire) > 0) {
                state.condVar.wait(state.mutex);
            }
        }

        pool->join();
    }

    STD_TEST(_SD) {
        for (volatile int i = 0; i < 1000000; i++) {
            doW(work);
        }
    }

    STD_TEST(_SS) {
        auto pool = ThreadPool::simple(16);
        StressState state(pool.mutPtr(), work);

        pool->submit(*new StressTask(&state, depth));

        {
            LockGuard lock(state.mutex);

            while (stdAtomicFetch(&state.counter, MemoryOrder::Acquire) > 0) {
                state.condVar.wait(state.mutex);
            }
        }

        pool->join();
    }
}
