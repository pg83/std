#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "cond_var.h"
#include "latch.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <functional>

using namespace stl;

namespace {
    static void doW(int work) {
        for (volatile int i = 0; i < work; ++i) {
        }
    }
}

STD_TEST_SUITE(CoroExecutor) {
    STD_TEST(Basic) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont*) {
            ++counter;
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(SingleYield) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont* c) {
            ++counter;
            c->executor()->yield();
            ++counter;
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(MultipleYields) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont* c) {
            for (int i = 0; i < 10; ++i) {
                ++counter;
                c->executor()->yield();
            }
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyCoros) {
        auto exec = CoroExecutor::create(4);
        Latch done(100);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&](Cont* c) {
                c->executor()->yield();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                done.arrive();
            });
        }

        done.wait();
        exec->pool()->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(NestedSpawn) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(2);
        int counter = 0;

        exec->spawn([&](Cont*) {
            exec->spawn([&](Cont*) {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                done.arrive();
            });

            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(YieldInterleave) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(2);

        auto fn = [&](Cont* c) {
            for (int i = 0; i < 5; ++i) {
                c->executor()->yield();
            }
            done.arrive();
        };

        exec->spawn(fn);
        exec->spawn(fn);

        done.wait();
        pool->join();
    }

    STD_TEST(MutexBasic) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int counter = 0;

        exec->spawn([&](Cont*) {
            Mutex mtx(exec.mutPtr());

            mtx.lock();
            ++counter;
            mtx.unlock();

            done.arrive();
        });

        done.wait();
        pool->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(MutexContention) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(2);
        int counter = 0;
        Mutex mtx(exec.mutPtr());

        for (int i = 0; i < 2; ++i) {
            exec->spawn([&](Cont* c) {
                for (int j = 0; j < 1000; ++j) {
                    LockGuard guard(mtx);
                    ++counter;
                }
                done.arrive();
            });
        }

        done.wait();
        pool->join();
        STD_INSIST(counter == 2000);
    }

    STD_TEST(MutexStress) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        const int nCoros = 16;
        const int nIters = 500;
        Latch done(nCoros);
        int counter = 0;
        Mutex mtx(exec.mutPtr());

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&](Cont* c) {
                for (int j = 0; j < nIters; ++j) {
                    LockGuard guard(mtx);
                    ++counter;
                }
                done.arrive();
            });
        }

        done.wait();
        pool->join();
        STD_INSIST(counter == nCoros * nIters);
    }

    STD_TEST(CondVarBasic) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        Latch done(1);
        int value = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        exec->spawn([&](Cont*) {
            LockGuard guard(mtx);
            while (value == 0) {
                cv.wait(mtx);
            }
            done.arrive();
        });

        exec->spawn([&](Cont*) {
            LockGuard guard(mtx);
            value = 1;
            cv.signal();
        });

        done.wait();
        pool->join();
        STD_INSIST(value == 1);
    }

    STD_TEST(CondVarBroadcast) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        const int N = 5;
        Latch done(N);
        int value = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        for (int i = 0; i < N; ++i) {
            exec->spawn([&](Cont*) {
                LockGuard guard(mtx);
                while (value == 0) {
                    cv.wait(mtx);
                }
                done.arrive();
            });
        }

        exec->spawn([&](Cont*) {
            LockGuard guard(mtx);
            value = 1;
            cv.broadcast();
        });

        done.wait();
        pool->join();
        STD_INSIST(value == 1);
    }

    STD_TEST(CondVarProducerConsumer) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        const int N = 10;
        Latch done(1);
        int produced = 0;
        int consumed = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        // consumer
        exec->spawn([&](Cont*) {
            for (int i = 0; i < N; ++i) {
                LockGuard guard(mtx);
                while (produced == consumed) {
                    cv.wait(mtx);
                }
                ++consumed;
            }
            done.arrive();
        });

        // producer
        exec->spawn([&](Cont*) {
            for (int i = 0; i < N; ++i) {
                LockGuard guard(mtx);
                ++produced;
                cv.signal();
            }
        });

        done.wait();
        pool->join();
        STD_INSIST(consumed == N);
    }

    STD_TEST(CondVarStress) {
        auto pool = ThreadPool::workStealing(4);
        auto exec = CoroExecutor::create(pool.mutPtr());
        const int nCoros = 8;
        const int nIters = 200;
        Latch done(nCoros);
        int value = 0;
        int woken = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&](Cont*) {
                for (int j = 0; j < nIters; ++j) {
                    LockGuard guard(mtx);
                    int cur = value;
                    while (value == cur) {
                        cv.wait(mtx);
                    }
                    ++woken;
                }
                done.arrive();
            });
        }

        exec->spawn([&](Cont*) {
            for (int j = 0; j < nCoros * nIters; ++j) {
                LockGuard guard(mtx);
                ++value;
                cv.broadcast();
            }
        });

        done.wait();
        pool->join();
        STD_INSIST(woken == nCoros * nIters);
    }

    const int depth = 22;
    const int work = 999;

    STD_TEST(_SW) {
        auto exec = CoroExecutor::create(16);

        int counter2 = 0;
        WaitGroup wg;

        std::function<void(Cont*, int)> run = [&](Cont* c, int d) {
            stdAtomicAddAndFetch(&counter2, 1, MemoryOrder::Relaxed);

            doW(work);

            if (d > 0) {
                wg.add(1);

                exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d]() {
                    run(exec->me(), d - 1);
                }));
            }

            c->executor()->yield();

            if (d > 0) {
                wg.add(1);

                exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d]() {
                    run(exec->me(), d - 1);
                }));
            }

            doW(work);

            wg.done();
        };

        wg.add(1);
        exec->spawn([&](Cont* c) {
            run(c, depth);
        });

        wg.wait();
        exec->pool()->join();

        _ctx.output() << counter2 << endL << flsH;
    }
}
