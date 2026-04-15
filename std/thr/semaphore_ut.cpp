#include "coro.h"
#include "thread.h"
#include "semaphore.h"

#include <std/tst/ut.h>
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

        ScopedThread t([&] {
            stdAtomicAddAndFetch(&value, 1, MemoryOrder::Release);
            sem.post();
        });

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

        ScopedThread t0(body), t1(body), t2(body), t3(body);
        ScopedThread t4(body), t5(body), t6(body), t7(body);

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
