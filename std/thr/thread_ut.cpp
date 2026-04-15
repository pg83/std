#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "runable.h"
#include "cond_var.h"

#include <std/tst/ut.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct CounterRunable: public Runable {
        int* counter;

        explicit CounterRunable(int* c)
            : counter(c)
        {
        }

        void run() noexcept override {
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

        void run() noexcept override {
            for (int i = 0; i < iterations; ++i) {
                ++(*counter);
            }
        }
    };

    struct IdRunable: public Runable {
        u64* id;

        explicit IdRunable(u64* id_ptr)
            : id(id_ptr)
        {
        }

        void run() noexcept override {
            *id = Thread::currentThreadId();
        }
    };
}

STD_TEST_SUITE(Thread) {
    STD_TEST(BasicThreadCreationAndJoin) {
        auto pool = ObjPool::fromMemory();
        int counter = 0;
        CounterRunable runnable(&counter);

        auto t = Thread::create(pool.mutPtr(), runnable);
        t->join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleThreads) {
        auto pool = ObjPool::fromMemory();
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        CounterRunable runnable1(&counter1);
        CounterRunable runnable2(&counter2);
        CounterRunable runnable3(&counter3);

        auto t1 = Thread::create(pool.mutPtr(), runnable1);
        auto t2 = Thread::create(pool.mutPtr(), runnable2);
        auto t3 = Thread::create(pool.mutPtr(), runnable3);

        t1->join();
        t2->join();
        t3->join();

        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
        STD_INSIST(counter3 == 1);
    }

    STD_TEST(ThreadWithMultipleIncrements) {
        auto pool = ObjPool::fromMemory();
        int counter = 0;
        MultiIncrementRunable runnable(&counter, 1000);

        auto t = Thread::create(pool.mutPtr(), runnable);
        t->join();

        STD_INSIST(counter == 1000);
    }

    STD_TEST(SequentialThreadCreation) {
        auto pool = ObjPool::fromMemory();

        for (int i = 0; i < 10; ++i) {
            int counter = 0;
            CounterRunable runnable(&counter);

            auto t = Thread::create(pool.mutPtr(), runnable);
            t->join();

            STD_INSIST(counter == 1);
        }
    }

    STD_TEST(MultipleThreadsWithDifferentWorkloads) {
        auto pool = ObjPool::fromMemory();
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        MultiIncrementRunable runnable1(&counter1, 100);
        MultiIncrementRunable runnable2(&counter2, 500);
        MultiIncrementRunable runnable3(&counter3, 1000);

        auto t1 = Thread::create(pool.mutPtr(), runnable1);
        auto t2 = Thread::create(pool.mutPtr(), runnable2);
        auto t3 = Thread::create(pool.mutPtr(), runnable3);

        t1->join();
        t2->join();
        t3->join();

        STD_INSIST(counter1 == 100);
        STD_INSIST(counter2 == 500);
        STD_INSIST(counter3 == 1000);
    }

    STD_TEST(ThreadJoinOrder) {
        auto pool = ObjPool::fromMemory();
        int counter1 = 0;
        int counter2 = 0;

        CounterRunable runnable1(&counter1);
        CounterRunable runnable2(&counter2);

        auto t1 = Thread::create(pool.mutPtr(), runnable1);
        auto t2 = Thread::create(pool.mutPtr(), runnable2);

        t2->join();
        t1->join();

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

            void run() noexcept override {
                *executed = true;
            }
        };

        auto pool = ObjPool::fromMemory();
        bool executed = false;
        EmptyRunable runnable(&executed);

        auto t = Thread::create(pool.mutPtr(), runnable);
        t->join();

        STD_INSIST(executed == true);
    }

    STD_TEST(ThreadId) {
        u64 id = 0;

        struct IdRunable: public Runable {
            u64* id;

            explicit IdRunable(u64* id)
                : id(id)
            {
            }

            void run() noexcept override {
                *id = Thread::currentThreadId();
            }
        };

        auto pool = ObjPool::fromMemory();
        IdRunable runnable(&id);
        auto t = Thread::create(pool.mutPtr(), runnable);
        u64 mainId = Thread::currentThreadId();
        t->join();

        STD_INSIST(id != 0);
        STD_INSIST(id != mainId);
    }

    STD_TEST(ThreadMethodThreadId) {
        auto pool = ObjPool::fromMemory();

        struct ThreadIdCollectorRunable: public Runable {
            u64* threadIdPtr;
            u64* currentThreadIdPtr;

            explicit ThreadIdCollectorRunable(u64* tidPtr, u64* curTidPtr)
                : threadIdPtr(tidPtr)
                , currentThreadIdPtr(curTidPtr)
            {
            }

            void run() noexcept override {
                *threadIdPtr = Thread::currentThreadId();
            }
        };

        u64 threadIdMethod = 0;
        u64 currentThreadId = 0;
        ThreadIdCollectorRunable runnable(&threadIdMethod, &currentThreadId);

        auto t = Thread::create(pool.mutPtr(), runnable);
        u64 fromMethod = t->threadId();
        t->join();

        STD_INSIST(threadIdMethod != 0);
        STD_INSIST(fromMethod != 0);
        STD_INSIST(fromMethod == threadIdMethod);
    }

    STD_TEST(DifferentThreadsHaveDifferentIds) {
        auto pool = ObjPool::fromMemory();
        u64 ids[5] = {0, 0, 0, 0, 0};

        struct IdCollectorRunable: public Runable {
            u64* id;

            explicit IdCollectorRunable(u64* i)
                : id(i)
            {
            }

            void run() noexcept override {
                *id = Thread::currentThreadId();
            }
        };

        IdCollectorRunable runables[] = {
            IdCollectorRunable(&ids[0]),
            IdCollectorRunable(&ids[1]),
            IdCollectorRunable(&ids[2]),
            IdCollectorRunable(&ids[3]),
            IdCollectorRunable(&ids[4])
        };

        auto t1 = Thread::create(pool.mutPtr(), runables[0]);
        auto t2 = Thread::create(pool.mutPtr(), runables[1]);
        auto t3 = Thread::create(pool.mutPtr(), runables[2]);
        auto t4 = Thread::create(pool.mutPtr(), runables[3]);
        auto t5 = Thread::create(pool.mutPtr(), runables[4]);

        t1->join();
        t2->join();
        t3->join();
        t4->join();
        t5->join();

        STD_INSIST(ids[0] != 0);
        STD_INSIST(ids[1] != 0);
        STD_INSIST(ids[2] != 0);
        STD_INSIST(ids[3] != 0);
        STD_INSIST(ids[4] != 0);

        STD_INSIST(ids[0] != ids[1] || ids[0] != ids[2] || ids[0] != ids[3] || ids[0] != ids[4]);
        STD_INSIST(ids[1] != ids[2] || ids[1] != ids[3] || ids[1] != ids[4]);
        STD_INSIST(ids[2] != ids[3] || ids[2] != ids[4]);
        STD_INSIST(ids[3] != ids[4]);
    }

    STD_TEST(ThreadWithMutex) {
        auto pool = ObjPool::fromMemory();
        int counter = 0;
        auto mutex = Mutex::create(pool.mutPtr());

        struct MutexRunable: public Runable {
            int* counter;
            Mutex* mutex;
            int iterations;

            MutexRunable(int* c, Mutex* m, int iter)
                : counter(c)
                , mutex(m)
                , iterations(iter)
            {
            }

            void run() noexcept override {
                for (int i = 0; i < iterations; ++i) {
                    LockGuard guard(*mutex);
                    ++(*counter);
                }
            }
        };

        MutexRunable runnable1(&counter, mutex, 1000);
        MutexRunable runnable2(&counter, mutex, 1000);
        MutexRunable runnable3(&counter, mutex, 1000);

        auto t1 = Thread::create(pool.mutPtr(), runnable1);
        auto t2 = Thread::create(pool.mutPtr(), runnable2);
        auto t3 = Thread::create(pool.mutPtr(), runnable3);

        t1->join();
        t2->join();
        t3->join();

        STD_INSIST(counter == 3000);
    }

    STD_TEST(ThreadWithCondVar) {
        auto pool = ObjPool::fromMemory();
        int counter = 0;
        auto mutex = Mutex::create(pool.mutPtr());
        auto cond = CondVar::create(pool.mutPtr());

        struct WaitRunable: public Runable {
            int* counter;
            Mutex* mutex;
            CondVar* cond;

            explicit WaitRunable(int* c, Mutex* m, CondVar* cv)
                : counter(c)
                , mutex(m)
                , cond(cv)
            {
            }

            void run() noexcept override {
                LockGuard guard(*mutex);
                while (*counter < 10) {
                    cond->wait(*mutex);
                }
            }
        };

        struct NotifyRunable: public Runable {
            int* counter;
            Mutex* mutex;
            CondVar* cond;

            explicit NotifyRunable(int* c, Mutex* m, CondVar* cv)
                : counter(c)
                , mutex(m)
                , cond(cv)
            {
            }

            void run() noexcept override {
                for (int i = 0; i < 10; ++i) {
                    {
                        LockGuard guard(*mutex);
                        ++(*counter);
                    }
                    cond->signal();
                }
            }
        };

        NotifyRunable notifyRunnable(&counter, mutex, cond);
        WaitRunable waitRunnable(&counter, mutex, cond);

        auto t1 = Thread::create(pool.mutPtr(), notifyRunnable);
        auto t2 = Thread::create(pool.mutPtr(), waitRunnable);

        t1->join();
        t2->join();

        STD_INSIST(counter == 10);
    }

    STD_TEST(LargeNumberOfIterations) {
        auto pool = ObjPool::fromMemory();
        int counter = 0;
        MultiIncrementRunable runnable(&counter, 100000);

        auto t = Thread::create(pool.mutPtr(), runnable);
        t->join();

        STD_INSIST(counter == 100000);
    }

    STD_TEST(ThreadWithZeroWork) {
        auto pool = ObjPool::fromMemory();

        struct ZeroWorkRunable: public Runable {
            bool* executed;

            explicit ZeroWorkRunable(bool* e)
                : executed(e)
            {
            }

            void run() noexcept override {
                *executed = true;
            }
        };

        bool executed = false;
        ZeroWorkRunable runnable(&executed);

        auto t = Thread::create(pool.mutPtr(), runnable);
        t->join();

        STD_INSIST(executed == true);
    }

    STD_TEST(ThreadMethodStart) {
        auto pool = ObjPool::fromMemory();
        int counter = 0;
        int counter2 = 0;

        CounterRunable runnable(&counter);
        CounterRunable runnable2(&counter2);

        auto t = Thread::create(pool.mutPtr(), runnable);
        t->join();

        auto t2 = Thread::create(pool.mutPtr(), runnable2);
        t2->join();

        STD_INSIST(counter == 1);
        STD_INSIST(counter2 == 1);
    }
}

STD_TEST_SUITE(CoroThread) {
    STD_TEST(BasicCreationAndJoin) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        CounterRunable runnable(&counter);

        exec->spawn([&] {
            auto t = Thread::create(pool.mutPtr(), exec, runnable);
            t->join();
        });

        exec->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleThreads) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter1 = 0, counter2 = 0, counter3 = 0;
        CounterRunable r1(&counter1), r2(&counter2), r3(&counter3);

        exec->spawn([&] {
            auto t1 = Thread::create(pool.mutPtr(), exec, r1);
            auto t2 = Thread::create(pool.mutPtr(), exec, r2);
            auto t3 = Thread::create(pool.mutPtr(), exec, r3);
            t1->join();
            t2->join();
            t3->join();
        });

        exec->join();
        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
        STD_INSIST(counter3 == 1);
    }

    STD_TEST(WithMultipleIncrements) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        MultiIncrementRunable runnable(&counter, 1000);

        exec->spawn([&] {
            auto t = Thread::create(pool.mutPtr(), exec, runnable);
            t->join();
        });

        exec->join();
        STD_INSIST(counter == 1000);
    }

    STD_TEST(SequentialCreation) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int total = 0;

        exec->spawn([&] {
            for (int i = 0; i < 10; ++i) {
                int counter = 0;
                CounterRunable runnable(&counter);
                auto t = Thread::create(pool.mutPtr(), exec, runnable);
                t->join();
                total += counter;
            }
        });

        exec->join();
        STD_INSIST(total == 10);
    }

    STD_TEST(MultipleWithDifferentWorkloads) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter1 = 0, counter2 = 0, counter3 = 0;
        MultiIncrementRunable r1(&counter1, 100);
        MultiIncrementRunable r2(&counter2, 500);
        MultiIncrementRunable r3(&counter3, 1000);

        exec->spawn([&] {
            auto t1 = Thread::create(pool.mutPtr(), exec, r1);
            auto t2 = Thread::create(pool.mutPtr(), exec, r2);
            auto t3 = Thread::create(pool.mutPtr(), exec, r3);
            t1->join();
            t2->join();
            t3->join();
        });

        exec->join();
        STD_INSIST(counter1 == 100);
        STD_INSIST(counter2 == 500);
        STD_INSIST(counter3 == 1000);
    }

    STD_TEST(JoinOrder) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter1 = 0, counter2 = 0;
        CounterRunable r1(&counter1), r2(&counter2);

        exec->spawn([&] {
            auto t1 = Thread::create(pool.mutPtr(), exec, r1);
            auto t2 = Thread::create(pool.mutPtr(), exec, r2);
            t2->join();
            t1->join();
        });

        exec->join();
        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
    }

    STD_TEST(ThreadId) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        u64 id = 0;
        bool executed = false;

        struct BoolRunable: public Runable {
            bool* executed;
            explicit BoolRunable(bool* e)
                : executed(e)
            {
            }
            void run() noexcept override {
                *executed = true;
            }
        };

        BoolRunable runnable(&executed);

        exec->spawn([&] {
            auto t = Thread::create(pool.mutPtr(), exec, runnable);
            id = t->threadId();
            t->join();
        });

        exec->join();
        STD_INSIST(executed == true);
        STD_INSIST(id != 0);
    }

    STD_TEST(DifferentThreadIds) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        u64 ids[5] = {0, 0, 0, 0, 0};

        exec->spawn([&] {
            for (int i = 0; i < 5; ++i) {
                IdRunable runnable(&ids[i]);
                auto t = Thread::create(pool.mutPtr(), exec, runnable);
                t->join();
            }
        });

        exec->join();

        for (int i = 0; i < 5; ++i) {
            STD_INSIST(ids[i] != 0);
        }
    }

    STD_TEST(ThreadWithMutex) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        auto mutex = Mutex::create(pool.mutPtr());

        struct MutexRunable: public Runable {
            int* counter;
            Mutex* mutex;
            int iterations;

            MutexRunable(int* c, Mutex* m, int iter)
                : counter(c)
                , mutex(m)
                , iterations(iter)
            {
            }

            void run() noexcept override {
                for (int i = 0; i < iterations; ++i) {
                    LockGuard guard(*mutex);
                    ++(*counter);
                }
            }
        };

        MutexRunable runnable1(&counter, mutex, 1000);
        MutexRunable runnable2(&counter, mutex, 1000);
        MutexRunable runnable3(&counter, mutex, 1000);

        exec->spawn([&] {
            auto t1 = Thread::create(pool.mutPtr(), exec, runnable1);
            auto t2 = Thread::create(pool.mutPtr(), exec, runnable2);
            auto t3 = Thread::create(pool.mutPtr(), exec, runnable3);
            t1->join();
            t2->join();
            t3->join();
        });

        exec->join();
        STD_INSIST(counter == 3000);
    }

    STD_TEST(LargeIterations) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        MultiIncrementRunable runnable(&counter, 100000);

        exec->spawn([&] {
            auto t = Thread::create(pool.mutPtr(), exec, runnable);
            t->join();
        });

        exec->join();
        STD_INSIST(counter == 100000);
    }

    STD_TEST(StartMethod) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        int counter2 = 0;

        CounterRunable runnable(&counter);
        CounterRunable runnable2(&counter2);

        exec->spawn([&] {
            auto t = Thread::create(pool.mutPtr(), exec, runnable);
            t->join();
            auto t2 = Thread::create(pool.mutPtr(), exec, runnable2);
            t2->join();
        });

        exec->join();
        STD_INSIST(counter == 1);
        STD_INSIST(counter2 == 1);
    }
}
