#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "cond_var.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(ThreadPool) {
    STD_TEST(BasicConstruction) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 4);
        pool->join();
    }

    STD_TEST(SingleTask) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;

        pool->submit([&counter] {
            ++counter;
        });
        pool->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleTasks) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;

        pool->submit([&counter] {
            ++counter;
        });
        pool->submit([&counter] {
            ++counter;
        });
        pool->submit([&counter] {
            ++counter;
        });
        pool->join();

        STD_INSIST(counter == 3);
    }

    STD_TEST(ManyTasksSingleThread) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;
        const int numTasks = 100;

        for (int i = 0; i < numTasks; ++i) {
            pool->submit([&counter] {
                ++counter;
            });
        }
        pool->join();

        STD_INSIST(counter == numTasks);
    }

    STD_TEST(ManyTasksMultipleThreads) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(EmptyPool) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 2);
        pool->join();
    }

    STD_TEST(TasksWithWork) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 4);

        for (int i = 0; i < 20; ++i) {
            pool->submit([] {
for (volatile int i = 0; i < 10000; i = i + 1) {} });
        }
        pool->join();
    }

    STD_TEST(SingleThreadPool) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;

        for (int i = 0; i < 10; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyThreadsPool) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 8);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 50);
    }
}

STD_TEST_SUITE(WorkStealingThreadPool) {
    STD_TEST(BasicConstruction) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 4);
        pool->join();
    }

    STD_TEST(SingleTask) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 1);
        int counter = 0;

        pool->submit([&counter] {
            ++counter;
        });
        pool->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleTasks) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 2);
        int counter = 0;

        for (int i = 0; i < 10; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyTasksSingleThread) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 1);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(ManyTasksMultipleThreads) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(EmptyPool) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 2);
        pool->join();
    }

    STD_TEST(TasksWithWork) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 4);

        for (int i = 0; i < 20; ++i) {
            pool->submit([] {
for (volatile int i = 0; i < 10000; i = i + 1) {} });
        }
        pool->join();
    }

    STD_TEST(SingleThreadPool) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 1);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 50);
    }

    STD_TEST(ManyThreadsPool) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 8);
        int counter = 0;

        for (int i = 0; i < 200; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 200);
    }

    STD_TEST(WorkStealing) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(MixedWorkload) {
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::workStealing(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        for (volatile int i = 0; i < 5000; i = i + 1) {
        }

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        pool->join();

        STD_INSIST(counter == 100);
    }
}


