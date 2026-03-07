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

STD_TEST_SUITE(TlsKeys) {
    STD_TEST(Unique) {
        u64 k1 = registerTlsKey();
        u64 k2 = registerTlsKey();
        u64 k3 = registerTlsKey();
        STD_INSIST(k1 != k2);
        STD_INSIST(k2 != k3);
        STD_INSIST(k1 != k3);
    }

    STD_TEST(Monotone) {
        u64 k1 = registerTlsKey();
        u64 k2 = registerTlsKey();
        STD_INSIST(k2 > k1);
    }

    STD_TEST(ConcurrentUnique) {
        const int N = 8;
        u64 keys[N] = {};
        int idx = 0;

        struct RegTask: Task {
            u64* keys;
            int* idx;
            RegTask(u64* k, int* i)
                : keys(k)
                , idx(i)
            {
            }
            void run() noexcept override {
                int i = stdAtomicAddAndFetch(idx, 1, MemoryOrder::Relaxed) - 1;
                keys[i] = registerTlsKey();
                delete this;
            }
        };

        auto pool = ThreadPool::simple(4);
        for (int i = 0; i < N; ++i) {
            pool->submitTask(*new RegTask(keys, &idx));
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
        auto pool = ThreadPool::sync();
        u64 key = registerTlsKey();
        STD_INSIST(pool->tls(key) != nullptr);
    }

    STD_TEST(StoreAndRetrieve) {
        auto pool = ThreadPool::sync();
        u64 key = registerTlsKey();
        int sentinel = 42;
        *pool->tls(key) = &sentinel;
        STD_INSIST(*pool->tls(key) == &sentinel);
    }

    STD_TEST(MultipleKeysIndependent) {
        auto pool = ThreadPool::sync();
        u64 k1 = registerTlsKey();
        u64 k2 = registerTlsKey();
        int v1 = 1, v2 = 2;
        *pool->tls(k1) = &v1;
        *pool->tls(k2) = &v2;
        STD_INSIST(*pool->tls(k1) == &v1);
        STD_INSIST(*pool->tls(k2) == &v2);
    }

    STD_TEST(PersistAcrossTasks) {
        auto pool = ThreadPool::sync();
        u64 key = registerTlsKey();
        int sentinel = 77;
        void* result = nullptr;

        struct ReadTask: Task {
            ThreadPool* pool;
            u64 key;
            void** out;
            ReadTask(ThreadPool* p, u64 k, void** o)
                : pool(p)
                , key(k)
                , out(o)
            {
            }
            void run() noexcept override {
                *out = *pool->tls(key);
            }
        };

        *pool->tls(key) = &sentinel;
        ReadTask task(pool.mutPtr(), key, &result);
        pool->submitTask(task);

        STD_INSIST(result == &sentinel);
    }
}

STD_TEST_SUITE(SimplePoolTls) {
    STD_TEST(NullFromOutside) {
        u64 key = registerTlsKey();
        auto pool = ThreadPool::simple(2);
        STD_INSIST(pool->tls(key) == nullptr);
        pool->join();
    }

    STD_TEST(NotNullFromTask) {
        u64 key = registerTlsKey();
        bool notNull = false;

        struct CheckTask: Task {
            ThreadPool* pool;
            u64 key;
            bool* out;
            CheckTask(ThreadPool* p, u64 k, bool* o)
                : pool(p)
                , key(k)
                , out(o)
            {
            }
            void run() noexcept override {
                *out = (pool->tls(key) != nullptr);
                delete this;
            }
        };

        auto pool = ThreadPool::simple(1);
        pool->submitTask(*new CheckTask(pool.mutPtr(), key, &notNull));
        pool->join();
        STD_INSIST(notNull);
    }

    STD_TEST(StoreAndRetrieve) {
        u64 key = registerTlsKey();
        int sentinel = 123;
        void* result = nullptr;

        struct SetTask: Task {
            ThreadPool* pool;
            u64 key;
            void* value;
            SetTask(ThreadPool* p, u64 k, void* v)
                : pool(p)
                , key(k)
                , value(v)
            {
            }
            void run() noexcept override {
                *pool->tls(key) = value;
                delete this;
            }
        };

        struct GetTask: Task {
            ThreadPool* pool;
            u64 key;
            void** out;
            GetTask(ThreadPool* p, u64 k, void** o)
                : pool(p)
                , key(k)
                , out(o)
            {
            }
            void run() noexcept override {
                *out = *pool->tls(key);
                delete this;
            }
        };

        // 1 thread => оба таска на одном воркере, TLS персистится
        auto pool = ThreadPool::simple(1);
        pool->submitTask(*new SetTask(pool.mutPtr(), key, &sentinel));
        pool->submitTask(*new GetTask(pool.mutPtr(), key, &result));
        pool->join();
        STD_INSIST(result == &sentinel);
    }

    STD_TEST(MultipleKeysIndependent) {
        u64 k1 = registerTlsKey();
        u64 k2 = registerTlsKey();
        int v1 = 1, v2 = 2;
        bool correct = true;

        struct CheckTask: Task {
            ThreadPool* pool;
            u64 k1, k2;
            void *val1, *val2;
            bool* correct;
            CheckTask(ThreadPool* p, u64 a, u64 b, void* va, void* vb, bool* c)
                : pool(p)
                , k1(a)
                , k2(b)
                , val1(va)
                , val2(vb)
                , correct(c)
            {
            }
            void run() noexcept override {
                *pool->tls(k1) = val1;
                *pool->tls(k2) = val2;
                if (*pool->tls(k1) != val1 || *pool->tls(k2) != val2) {
                    *correct = false;
                }
                delete this;
            }
        };

        auto pool = ThreadPool::simple(1);
        pool->submitTask(*new CheckTask(pool.mutPtr(), k1, k2, &v1, &v2, &correct));
        pool->join();
        STD_INSIST(correct);
    }

    STD_TEST(WorkerIsolation) {
        const int N = 2;
        u64 key = registerTlsKey();

        struct Barrier {
            Mutex mutex;
            CondVar cv;
            int count = 0;
            int total;
            explicit Barrier(int n)
                : total(n)
            {
            }
            void wait() noexcept {
                LockGuard lock(mutex);
                if (++count >= total) {
                    cv.broadcast();
                } else {
                    while (count < total) {
                        cv.wait(mutex);
                    }
                }
            }
        };

        Barrier barrier(N);
        bool correct = true;

        struct IsoTask: Task {
            ThreadPool* pool;
            u64 key;
            Barrier* barrier;
            int myId;
            bool* correct;
            IsoTask(ThreadPool* p, u64 k, Barrier* b, int id, bool* c)
                : pool(p)
                , key(k)
                , barrier(b)
                , myId(id)
                , correct(c)
            {
            }
            void run() noexcept override {
                *pool->tls(key) = (void*)(uintptr_t)myId;
                barrier->wait();
                if ((uintptr_t)*pool->tls(key) != (uintptr_t)myId) {
                    *correct = false;
                }
                delete this;
            }
        };

        auto pool = ThreadPool::simple(N);
        for (int i = 0; i < N; ++i) {
            pool->submitTask(*new IsoTask(pool.mutPtr(), key, &barrier, i + 1, &correct));
        }
        pool->join();
        STD_INSIST(correct);
    }
}

STD_TEST_SUITE(WorkStealingPoolTls) {
    STD_TEST(NullFromOutside) {
        u64 key = registerTlsKey();
        auto pool = ThreadPool::workStealing(2);
        STD_INSIST(pool->tls(key) == nullptr);
        pool->join();
    }

    STD_TEST(NotNullFromTask) {
        u64 key = registerTlsKey();
        bool notNull = false;

        struct CheckTask: Task {
            ThreadPool* pool;
            u64 key;
            bool* out;
            CheckTask(ThreadPool* p, u64 k, bool* o)
                : pool(p)
                , key(k)
                , out(o)
            {
            }
            void run() noexcept override {
                *out = (pool->tls(key) != nullptr);
                delete this;
            }
        };

        auto pool = ThreadPool::workStealing(2);
        pool->submitTask(*new CheckTask(pool.mutPtr(), key, &notNull));
        pool->join();
        STD_INSIST(notNull);
    }

    STD_TEST(MultipleKeysIndependent) {
        u64 k1 = registerTlsKey();
        u64 k2 = registerTlsKey();
        int v1 = 10, v2 = 20;
        bool correct = true;

        struct CheckTask: Task {
            ThreadPool* pool;
            u64 k1, k2;
            void *val1, *val2;
            bool* correct;
            CheckTask(ThreadPool* p, u64 a, u64 b, void* va, void* vb, bool* c)
                : pool(p)
                , k1(a)
                , k2(b)
                , val1(va)
                , val2(vb)
                , correct(c)
            {
            }
            void run() noexcept override {
                *pool->tls(k1) = val1;
                *pool->tls(k2) = val2;
                if (*pool->tls(k1) != val1 || *pool->tls(k2) != val2) {
                    *correct = false;
                }
                delete this;
            }
        };

        auto pool = ThreadPool::workStealing(2);
        pool->submitTask(*new CheckTask(pool.mutPtr(), k1, k2, &v1, &v2, &correct));
        pool->join();
        STD_INSIST(correct);
    }

    STD_TEST(WorkerIsolation) {
        const int N = 2;
        u64 key = registerTlsKey();

        struct Barrier {
            Mutex mutex;
            CondVar cv;
            int count = 0;
            int total;
            explicit Barrier(int n)
                : total(n)
            {
            }
            void wait() noexcept {
                LockGuard lock(mutex);
                if (++count >= total) {
                    cv.broadcast();
                } else {
                    while (count < total) {
                        cv.wait(mutex);
                    }
                }
            }
        };

        Barrier barrier(N);
        Barrier done(N + 1);
        bool correct = true;

        struct IsoTask: Task {
            ThreadPool* pool;
            u64 key;
            Barrier* barrier;
            Barrier* done;
            int myId;
            bool* correct;
            IsoTask(ThreadPool* p, u64 k, Barrier* b, Barrier* d, int id, bool* c)
                : pool(p)
                , key(k)
                , barrier(b)
                , done(d)
                , myId(id)
                , correct(c)
            {
            }
            void run() noexcept override {
                *pool->tls(key) = (void*)(uintptr_t)myId;
                barrier->wait();
                if ((uintptr_t)*pool->tls(key) != (uintptr_t)myId) {
                    *correct = false;
                }
                done->wait();
                delete this;
            }
        };

        auto pool = ThreadPool::workStealing(N);
        for (int i = 0; i < N; ++i) {
            pool->submitTask(*new IsoTask(pool.mutPtr(), key, &barrier, &done, i + 1, &correct));
        }
        done.wait();
        pool->join();
        STD_INSIST(correct);
    }
}
