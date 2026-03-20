#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "cond_var.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    static void doW(int work) {
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

    struct StressTask {
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
            auto* t = new StressTask(state_, depth_ - 1);
            state_->pool->submit([t] { t->run(); });
        }

        void run() noexcept {
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
}

STD_TEST_SUITE(ThreadPool) {
    STD_TEST(BasicConstruction) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);
        pool->join();
    }

    STD_TEST(SingleTask) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;

        pool->submit([&counter] { ++counter; });
        pool->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleTasks) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;

        pool->submit([&counter] { ++counter; });
        pool->submit([&counter] { ++counter; });
        pool->submit([&counter] { ++counter; });
        pool->join();

        STD_INSIST(counter == 3);
    }

    STD_TEST(ManyTasksSingleThread) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;
        const int numTasks = 100;

        for (int i = 0; i < numTasks; ++i) {
            pool->submit([&counter] { ++counter; });
        }
        pool->join();

        STD_INSIST(counter == numTasks);
    }

    STD_TEST(ManyTasksMultipleThreads) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] { stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(EmptyPool) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 2);
        pool->join();
    }

    STD_TEST(TasksWithWork) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);

        for (int i = 0; i < 20; ++i) {
            pool->submit([] { for (volatile int i = 0; i < 10000; ++i) {} });
        }
        pool->join();
    }

    STD_TEST(SingleThreadPool) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        int counter = 0;

        for (int i = 0; i < 10; ++i) {
            pool->submit([&counter] { stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        }
        pool->join();

        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyThreadsPool) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 8);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] { stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }
}

STD_TEST_SUITE(WorkStealingThreadPool) {
    STD_TEST(BasicConstruction) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 4);
        pool->join();
    }

    STD_TEST(SingleTask) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 1);
        int counter = 0;

        pool->submit([&counter] { ++counter; });
        pool->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleTasks) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 2);
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
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 1);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter] { stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        }
        pool->join();

        STD_INSIST(counter == 100);
    }

    STD_TEST(ManyTasksMultipleThreads) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 4);
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
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 2);
        pool->join();
    }

    STD_TEST(TasksWithWork) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 4);

        for (int i = 0; i < 20; ++i) {
            pool->submit([] { for (volatile int i = 0; i < 10000; ++i) {} });
        }
        pool->join();
    }

    STD_TEST(SingleThreadPool) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 1);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] { stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        }
        pool->join();

        STD_INSIST(counter == 50);
    }

    STD_TEST(ManyThreadsPool) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 8);
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
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 4);
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
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        for (volatile int i = 0; i < 5000; ++i) {
        }

        for (int i = 0; i < 50; ++i) {
            pool->submit([&counter] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        pool->join();

        STD_INSIST(counter == 100);
    }

    const int depth = 25;
    const int work = 250;

    STD_TEST(_SW) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 16);
        StressState state(pool, work);

        auto* task = new StressTask(&state, depth);
        pool->submit([task] { task->run(); });
        pool->join();
    }

    STD_TEST(_SD) {
        for (volatile int i = 0; i < 10000000; i++) {
            doW(work);
        }
    }

    STD_TEST(_SS) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 16);
        StressState state(pool, work);

        auto* task = new StressTask(&state, depth);
        pool->submit([task] { task->run(); });

        {
            LockGuard lock(state.mutex);

            while (stdAtomicFetch(&state.counter, MemoryOrder::Acquire) > 0) {
                state.condVar.wait(state.mutex);
            }
        }

        pool->join();
    }
}

STD_TEST_SUITE(TlsKeys) {
    STD_TEST(Unique) {
        u64 k1 = ThreadPool::registerTlsKey();
        u64 k2 = ThreadPool::registerTlsKey();
        u64 k3 = ThreadPool::registerTlsKey();
        STD_INSIST(k1 != k2);
        STD_INSIST(k2 != k3);
        STD_INSIST(k1 != k3);
    }

    STD_TEST(Monotone) {
        u64 k1 = ThreadPool::registerTlsKey();
        u64 k2 = ThreadPool::registerTlsKey();
        STD_INSIST(k2 > k1);
    }

    STD_TEST(ConcurrentUnique) {
        const int N = 8;
        u64 keys[N] = {};
        int idx = 0;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 4);
        for (int i = 0; i < N; ++i) {
            pool->submit([&keys, &idx] {
                int i = stdAtomicAddAndFetch(&idx, 1, MemoryOrder::Relaxed) - 1;
                keys[i] = ThreadPool::registerTlsKey();
            });
        }
        pool->join();

        for (int i = 0; i < N; ++i) {
            for (int j = i + 1; j < N; ++j) {
                STD_INSIST(keys[i] != keys[j]);
            }
        }
    }
}

STD_TEST_SUITE(SyncPoolTls) {
    STD_TEST(NotNull) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::sync(opool.mutPtr());
        u64 key = ThreadPool::registerTlsKey();
        STD_INSIST(pool->tls(key) != nullptr);
    }

    STD_TEST(StoreAndRetrieve) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::sync(opool.mutPtr());
        u64 key = ThreadPool::registerTlsKey();
        int sentinel = 42;
        *pool->tls(key) = &sentinel;
        STD_INSIST(*pool->tls(key) == &sentinel);
    }

    STD_TEST(MultipleKeysIndependent) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::sync(opool.mutPtr());
        u64 k1 = ThreadPool::registerTlsKey();
        u64 k2 = ThreadPool::registerTlsKey();
        int v1 = 1, v2 = 2;
        *pool->tls(k1) = &v1;
        *pool->tls(k2) = &v2;
        STD_INSIST(*pool->tls(k1) == &v1);
        STD_INSIST(*pool->tls(k2) == &v2);
    }

    STD_TEST(PersistAcrossTasks) {
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::sync(opool.mutPtr());
        u64 key = ThreadPool::registerTlsKey();
        int sentinel = 77;
        void* result = nullptr;

        *pool->tls(key) = &sentinel;
        pool->submit([pool, key, &result] { result = *pool->tls(key); });

        STD_INSIST(result == &sentinel);
    }
}

STD_TEST_SUITE(SimplePoolTls) {
    STD_TEST(NullFromOutside) {
        u64 key = ThreadPool::registerTlsKey();
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 2);
        STD_INSIST(pool->tls(key) == nullptr);
        pool->join();
    }

    STD_TEST(NotNullFromTask) {
        u64 key = ThreadPool::registerTlsKey();
        bool notNull = false;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        pool->submit([pool, key, &notNull] { notNull = (pool->tls(key) != nullptr); });
        pool->join();
        STD_INSIST(notNull);
    }

    STD_TEST(StoreAndRetrieve) {
        u64 key = ThreadPool::registerTlsKey();
        int sentinel = 123;
        void* result = nullptr;

        // 1 thread => оба таска на одном воркере, TLS персистится
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        pool->submit([pool, key, &sentinel] { *pool->tls(key) = &sentinel; });
        pool->submit([pool, key, &result] { result = *pool->tls(key); });
        pool->join();
        STD_INSIST(result == &sentinel);
    }

    STD_TEST(MultipleKeysIndependent) {
        u64 k1 = ThreadPool::registerTlsKey();
        u64 k2 = ThreadPool::registerTlsKey();
        int v1 = 1, v2 = 2;
        bool correct = true;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), 1);
        pool->submit([pool, k1, k2, &v1, &v2, &correct] {
            *pool->tls(k1) = &v1;
            *pool->tls(k2) = &v2;
            if (*pool->tls(k1) != &v1 || *pool->tls(k2) != &v2) {
                correct = false;
            }
        });
        pool->join();
        STD_INSIST(correct);
    }

    STD_TEST(WorkerIsolation) {
        const int N = 2;
        u64 key = ThreadPool::registerTlsKey();
        WaitGroup wg(N);
        bool correct = true;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::simple(opool.mutPtr(), N);
        for (int i = 0; i < N; ++i) {
            pool->submit([pool, key, &wg, id = i + 1, &correct] {
                *pool->tls(key) = (void*)(uintptr_t)id;
                wg.done();
                wg.wait();
                if ((uintptr_t)*pool->tls(key) != (uintptr_t)id) {
                    correct = false;
                }
            });
        }
        pool->join();
        STD_INSIST(correct);
    }
}

STD_TEST_SUITE(WorkStealingPoolTls) {
    STD_TEST(NullFromOutside) {
        u64 key = ThreadPool::registerTlsKey();
        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 2);
        STD_INSIST(pool->tls(key) == nullptr);
        pool->join();
    }

    STD_TEST(NotNullFromTask) {
        u64 key = ThreadPool::registerTlsKey();
        bool notNull = false;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 2);
        pool->submit([pool, key, &notNull] {
            notNull = (pool->tls(key) != nullptr);
        });
        pool->join();
        STD_INSIST(notNull);
    }

    STD_TEST(MultipleKeysIndependent) {
        u64 k1 = ThreadPool::registerTlsKey();
        u64 k2 = ThreadPool::registerTlsKey();
        int v1 = 10, v2 = 20;
        bool correct = true;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), 2);
        pool->submit([pool, k1, k2, &v1, &v2, &correct] {
            *pool->tls(k1) = &v1;
            *pool->tls(k2) = &v2;
            if (*pool->tls(k1) != &v1 || *pool->tls(k2) != &v2) {
                correct = false;
            }
        });
        pool->join();
        STD_INSIST(correct);
    }

    STD_TEST(_WorkerIsolation) {
        const int N = 2;
        u64 key = ThreadPool::registerTlsKey();
        WaitGroup wg(N);
        bool correct = true;

        auto opool = ObjPool::fromMemory();
        auto* pool = ThreadPool::workStealing(opool.mutPtr(), N);
        for (int i = 0; i < N; ++i) {
            pool->submit([pool, key, &wg, id = i + 1, &correct] {
                *pool->tls(key) = (void*)(uintptr_t)id;
                wg.done();
                wg.wait();
                if ((uintptr_t)*pool->tls(key) != (uintptr_t)id) {
                    correct = false;
                }
            });
        }
        pool->join();
        STD_INSIST(correct);
    }
}
