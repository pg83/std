#include "coro.h"
#include "thread.h"
#include "runable.h"
#include "semaphore.h"

#include <std/tst/ut.h>
#include <std/alg/defer.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(Semaphore) {
    STD_TEST(PostThenWait) {
        auto pool = ObjPool::fromMemory();
        auto& sem = *Semaphore::create(pool.mutPtr(), 0);

        sem.post();
        sem.wait();
    }

    STD_TEST(InitialCount) {
        auto pool = ObjPool::fromMemory();
        auto& sem = *Semaphore::create(pool.mutPtr(), 3);

        sem.wait();
        sem.wait();
        sem.wait();

        sem.post();
        sem.post();
        sem.post();
    }

    STD_TEST(TwoThreads) {
        auto pool = ObjPool::fromMemory();
        auto& sem = *Semaphore::create(pool.mutPtr(), 0);
        int value = 0;

        auto r = makeRunable([&] {
            stdAtomicAddAndFetch(&value, 1, MemoryOrder::Release);
            sem.post();
        });

        auto* t = Thread::create(pool.mutPtr(), r);
        STD_DEFER {
            t->join();
        };

        sem.wait();
        STD_INSIST(stdAtomicFetch(&value, MemoryOrder::Acquire) == 1);
    }

    STD_TEST(ManyThreads) {
        const int N = 8;
        auto pool = ObjPool::fromMemory();
        auto& sem = *Semaphore::create(pool.mutPtr(), 0);
        int counter = 0;

        auto body = [&] {
            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            sem.post();
        };

        auto r0 = makeRunable(body);
        auto r1 = makeRunable(body);
        auto r2 = makeRunable(body);
        auto r3 = makeRunable(body);
        auto r4 = makeRunable(body);
        auto r5 = makeRunable(body);
        auto r6 = makeRunable(body);
        auto r7 = makeRunable(body);

        auto* t0 = Thread::create(pool.mutPtr(), r0);
        STD_DEFER {
            t0->join();
        };
        auto* t1 = Thread::create(pool.mutPtr(), r1);
        STD_DEFER {
            t1->join();
        };
        auto* t2 = Thread::create(pool.mutPtr(), r2);
        STD_DEFER {
            t2->join();
        };
        auto* t3 = Thread::create(pool.mutPtr(), r3);
        STD_DEFER {
            t3->join();
        };
        auto* t4 = Thread::create(pool.mutPtr(), r4);
        STD_DEFER {
            t4->join();
        };
        auto* t5 = Thread::create(pool.mutPtr(), r5);
        STD_DEFER {
            t5->join();
        };
        auto* t6 = Thread::create(pool.mutPtr(), r6);
        STD_DEFER {
            t6->join();
        };
        auto* t7 = Thread::create(pool.mutPtr(), r7);
        STD_DEFER {
            t7->join();
        };

        for (int i = 0; i < N; ++i) {
            sem.wait();
        }

        STD_INSIST(counter == N);
    }

    STD_TEST(CoroBasic) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& sem = *Semaphore::create(pool.mutPtr(), 0, exec);

        exec->spawn([&] {
            exec->spawn([&] {
                sem.post();
            });

            sem.wait();
        });

        exec->join();
    }

    STD_TEST(CoroInitialCount) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& sem = *Semaphore::create(pool.mutPtr(), 3, exec);

        exec->spawn([&] {
            sem.wait();
            sem.wait();
            sem.wait();

            sem.post();
            sem.post();
            sem.post();
        });

        exec->join();
    }

    STD_TEST(CoroMultiple) {
        const int N = 8;
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& sem = *Semaphore::create(pool.mutPtr(), 0, exec);
        int counter = 0;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                exec->spawn([&] {
                    stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                    sem.post();
                });
            }

            for (int i = 0; i < N; ++i) {
                sem.wait();
            }

            STD_INSIST(counter == N);
        });

        exec->join();
    }
}
