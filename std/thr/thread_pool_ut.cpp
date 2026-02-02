#include "thread_pool.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace Std;

namespace {
    struct CounterTask : public Task {
        int* counter_;
        
        explicit CounterTask(int* c)
            : counter_(c)
        {
        }
        
        void run() noexcept override {
            ++(*counter_);
        }
    };
    
    struct AtomicCounterTask : public Task {
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
    
    struct SleepTask : public Task {
        void run() noexcept override {
            for (volatile int i = 0; i < 10000; ++i) {
            }
        }
    };
}

STD_TEST_SUITE(ThreadPool) {
    STD_TEST(BasicConstruction) {
        ThreadPool pool(4);
        pool.join();
    }
    
    STD_TEST(SingleTask) {
        ThreadPool pool(1);
        int counter = 0;
        
        CounterTask task(&counter);
        pool.submit(&task);
        pool.join();
        
        STD_INSIST(counter == 1);
    }
    
    STD_TEST(MultipleTasks) {
        ThreadPool pool(2);
        int counter = 0;
        
        CounterTask task1(&counter);
        CounterTask task2(&counter);
        CounterTask task3(&counter);
        
        pool.submit(&task1);
        pool.submit(&task2);
        pool.submit(&task3);
        pool.join();
        
        STD_INSIST(counter == 3);
    }
    
    STD_TEST(ManyTasksSingleThread) {
        ThreadPool pool(1);
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
            CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter), CounterTask(&counter)
        };
        
        for (int i = 0; i < numTasks; ++i) {
            pool.submit(&tasks[i]);
        }
        pool.join();
        
        STD_INSIST(counter == numTasks);
    }
    
    STD_TEST(ManyTasksMultipleThreads) {
        ThreadPool pool(4);
        int counter = 0;
        
        for (int i = 0; i < 100; ++i) {
            pool.submit(new AtomicCounterTask(&counter));
        }
        pool.join();
        
        STD_INSIST(counter == 100);
    }
    
    STD_TEST(EmptyPool) {
        ThreadPool pool(2);
        pool.join();
    }
    
    STD_TEST(TasksWithWork) {
        ThreadPool pool(4);
        
        SleepTask tasks[20] = {
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask(),
            SleepTask(), SleepTask(), SleepTask(), SleepTask(), SleepTask()
        };
        
        for (int i = 0; i < 20; ++i) {
            pool.submit(&tasks[i]);
        }
        pool.join();
    }
    
    STD_TEST(SingleThreadPool) {
        ThreadPool pool(1);
        int counter = 0;
        
        for (int i = 0; i < 10; ++i) {
            pool.submit(new AtomicCounterTask(&counter));
        }
        pool.join();
        
        STD_INSIST(counter == 10);
    }
    
    STD_TEST(ManyThreadsPool) {
        ThreadPool pool(8);
        int counter = 0;
        
        for (int i = 0; i < 100; ++i) {
            pool.submit(new AtomicCounterTask(&counter));
        }
        pool.join();
        
        STD_INSIST(counter == 100);
    }
}
