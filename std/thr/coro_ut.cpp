#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "latch.h"
#include "cond_var.h"
#include "barrier.h"
#include "wait_group.h"
#include "poller.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

#include <functional>
#include <unistd.h>

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
        int queue = 0;
        int consumed = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&](Cont*) {
                for (int j = 0; j < nIters; ++j) {
                    LockGuard guard(mtx);
                    while (queue == 0) {
                        cv.wait(mtx);
                    }
                    --queue;
                    ++consumed;
                }
                done.arrive();
            });
        }

        exec->spawn([&](Cont*) {
            for (int j = 0; j < nCoros * nIters; ++j) {
                LockGuard guard(mtx);
                ++queue;
                cv.signal();
            }
        });

        done.wait();
        pool->join();
        STD_INSIST(consumed == nCoros * nIters);
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

STD_TEST_SUITE(CoroPoll) {
    STD_TEST(PipeReadWrite) {
        // reader polls In on pipe, writer writes — verify data arrives
        auto exec = CoroExecutor::create(4);
        Latch done(1);
        int pipefd[2];
        STD_INSIST(::pipe(pipefd) == 0);
        int result = 0;

        exec->spawn([&](Cont* c) {
            u32 ready = c->poll(pipefd[0], PollFlag::In, 2000);
            STD_INSIST(ready & PollFlag::In);
            char buf;
            ::read(pipefd[0], &buf, 1);
            result = buf;
            done.arrive();
        });

        exec->spawn([&](Cont*) {
            char b = 42;
            ::write(pipefd[1], &b, 1);
        });

        done.wait();
        exec->pool()->join();
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        STD_INSIST(result == 42);
    }

    STD_TEST(Timeout) {
        // poll on idle pipe with short timeout — must return 0 and ~100ms elapsed
        auto exec = CoroExecutor::create(4);
        Latch done(1);
        u32 pollResult = 1;
        int pipefd[2];
        STD_INSIST(::pipe(pipefd) == 0);

        exec->spawn([&](Cont* c) {
            pollResult = c->poll(pipefd[0], PollFlag::In, 100);
            done.arrive();
        });

        done.wait();
        exec->pool()->join();
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        STD_INSIST(pollResult == 0);
    }

    STD_TEST(MultiPipe) {
        // N coroutines each poll their own pipe, woken in reverse order
        auto exec = CoroExecutor::create(4);
        const int N = 8;
        Latch done(N);
        int pipes[N][2];
        int order[N];
        int orderIdx = 0;

        for (int i = 0; i < N; i++) {
            STD_INSIST(::pipe(pipes[i]) == 0);
        }

        for (int i = 0; i < N; i++) {
            exec->spawn([&, i](Cont* c) {
                u32 ready = c->poll(pipes[i][0], PollFlag::In, 2000);
                STD_INSIST(ready & PollFlag::In);
                char buf;
                ::read(pipes[i][0], &buf, 1);
                order[stdAtomicAddAndFetch(&orderIdx, 1, MemoryOrder::Relaxed) - 1] = i;
                done.arrive();
            });
        }

        // wake in reverse order
        exec->spawn([&](Cont*) {
            for (int i = N - 1; i >= 0; i--) {
                char b = 1;
                ::write(pipes[i][1], &b, 1);
            }
        });

        done.wait();
        exec->pool()->join();

        for (int i = 0; i < N; i++) {
            ::close(pipes[i][0]);
            ::close(pipes[i][1]);
        }
        // all N coroutines woke up
        STD_INSIST(orderIdx == N);
    }
}
