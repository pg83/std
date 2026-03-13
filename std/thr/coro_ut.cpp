#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "cond_var.h"
#include "poller.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <std/sys/fd.h>

#include <errno.h>
#include <unistd.h>

#undef noexcept

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
        auto exec = CoroExecutor::create(4);
        int counter = 0;

        exec->spawn([&]() {
            ++counter;
        });

        exec->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(SingleYield) {
        auto exec = CoroExecutor::create(4);
        int counter = 0;

        exec->spawn([&]() {
            ++counter;
            exec->yield();
            ++counter;
        });

        exec->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(MultipleYields) {
        auto exec = CoroExecutor::create(4);
        int counter = 0;

        exec->spawn([&]() {
            for (int i = 0; i < 10; ++i) {
                ++counter;
                exec->yield();
            }
        });

        exec->join();
        STD_INSIST(counter == 10);
    }

    STD_TEST(ManyCoros) {
        auto exec = CoroExecutor::create(4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&]() {
                exec->yield();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        exec->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(NestedSpawn) {
        auto exec = CoroExecutor::create(4);
        int counter = 0;

        exec->spawn([&]() {
            exec->spawn([&]() {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });

            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
        });

        exec->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(YieldInterleave) {
        auto exec = CoroExecutor::create(4);

        auto fn = [&]() {
            for (int i = 0; i < 5; ++i) {
                exec->yield();
            }
        };

        exec->spawn(fn);
        exec->spawn(fn);

        exec->join();
    }

    STD_TEST(MutexBasic) {
        auto exec = CoroExecutor::create(4);
        int counter = 0;

        exec->spawn([&]() {
            Mutex mtx(exec.mutPtr());

            mtx.lock();
            ++counter;
            mtx.unlock();
        });

        exec->join();
        STD_INSIST(counter == 1);
    }

    STD_TEST(MutexContention) {
        auto exec = CoroExecutor::create(4);
        int counter = 0;
        Mutex mtx(exec.mutPtr());

        for (int i = 0; i < 2; ++i) {
            exec->spawn([&]() {
                for (int j = 0; j < 1000; ++j) {
                    LockGuard guard(mtx);
                    ++counter;
                }
            });
        }

        exec->join();
        STD_INSIST(counter == 2000);
    }

    STD_TEST(MutexStress) {
        auto exec = CoroExecutor::create(4);
        const int nCoros = 16;
        const int nIters = 500;
        int counter = 0;
        Mutex mtx(exec.mutPtr());

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&]() {
                for (int j = 0; j < nIters; ++j) {
                    LockGuard guard(mtx);
                    ++counter;
                }
            });
        }

        exec->join();
        STD_INSIST(counter == nCoros * nIters);
    }

    STD_TEST(CondVarBasic) {
        auto exec = CoroExecutor::create(4);
        int value = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        exec->spawn([&]() {
            LockGuard guard(mtx);
            while (value == 0) {
                cv.wait(mtx);
            }
        });

        exec->spawn([&]() {
            LockGuard guard(mtx);
            value = 1;
            cv.signal();
        });

        exec->join();
        STD_INSIST(value == 1);
    }

    STD_TEST(CondVarBroadcast) {
        auto exec = CoroExecutor::create(4);
        const int N = 5;
        int value = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        for (int i = 0; i < N; ++i) {
            exec->spawn([&]() {
                LockGuard guard(mtx);
                while (value == 0) {
                    cv.wait(mtx);
                }
            });
        }

        exec->spawn([&]() {
            LockGuard guard(mtx);
            value = 1;
            cv.broadcast();
        });

        exec->join();
        STD_INSIST(value == 1);
    }

    STD_TEST(CondVarProducerConsumer) {
        auto exec = CoroExecutor::create(4);
        const int N = 10;
        int produced = 0;
        int consumed = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        // consumer
        exec->spawn([&]() {
            for (int i = 0; i < N; ++i) {
                LockGuard guard(mtx);
                while (produced == consumed) {
                    cv.wait(mtx);
                }
                ++consumed;
            }
        });

        // producer
        exec->spawn([&]() {
            for (int i = 0; i < N; ++i) {
                LockGuard guard(mtx);
                ++produced;
                cv.signal();
            }
        });

        exec->join();
        STD_INSIST(consumed == N);
    }

    STD_TEST(CondVarStress) {
        auto exec = CoroExecutor::create(4);
        const int nCoros = 8;
        const int nIters = 200;
        int queue = 0;
        int consumed = 0;
        Mutex mtx(exec.mutPtr());
        CondVar cv(exec.mutPtr());

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&]() {
                for (int j = 0; j < nIters; ++j) {
                    LockGuard guard(mtx);
                    while (queue == 0) {
                        cv.wait(mtx);
                    }
                    --queue;
                    ++consumed;
                }
            });
        }

        exec->spawn([&]() {
            for (int j = 0; j < nCoros * nIters; ++j) {
                LockGuard guard(mtx);
                ++queue;
                cv.signal();
            }
        });

        exec->join();
        STD_INSIST(consumed == nCoros * nIters);
    }

    const int depth = 22;
    const int work = 999;

    STD_TEST(_SW) {
        auto exec = CoroExecutor::create(16);

        int counter2 = 0;

        std::function<void(int)> run = [&](int d) {
            stdAtomicAddAndFetch(&counter2, 1, MemoryOrder::Relaxed);

            doW(work);

            if (d > 0) {
                exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d]() {
                    run(d - 1);
                }));
            }

            exec->yield();

            if (d > 0) {
                exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d]() {
                    run(d - 1);
                }));
            }

            doW(work);
        };

        exec->spawn([&]() {
            run(depth);
        });

        exec->join();
    }
}

STD_TEST_SUITE(CoroRandom) {
    STD_TEST(NonZero) {
        auto exec = CoroExecutor::create(4);
        u32 result = 0;

        exec->spawn([&]() {
            result = exec->random();
        });

        exec->join();
        STD_INSIST(result != 0);
    }

    STD_TEST(DifferentPerCoro) {
        auto exec = CoroExecutor::create(4);
        const int N = 8;
        u32 values[N] = {};

        for (int i = 0; i < N; ++i) {
            exec->spawn([&, i]() {
                values[i] = exec->random();
            });
        }

        exec->join();

        for (int i = 0; i < N; ++i) {
            for (int j = i + 1; j < N; ++j) {
                STD_INSIST(values[i] != values[j]);
            }
        }
    }

    STD_TEST(Successive) {
        auto exec = CoroExecutor::create(4);
        u32 a = 0, b = 0;

        exec->spawn([&]() {
            a = exec->random();
            b = exec->random();
        });

        exec->join();
        STD_INSIST(a != b);
    }
}

STD_TEST_SUITE(CoroPoll) {
    STD_TEST(PipeReadWrite) {
        // reader polls In on pipe, writer writes — verify data arrives
        auto exec = CoroExecutor::create(4);
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);
        int result = 0;

        exec->spawn([&]() {
            u32 ready = exec->poll(readEnd.get(), PollFlag::In);
            STD_INSIST(ready & PollFlag::In);
            char buf;
            readEnd.read(&buf, 1);
            result = buf;
        });

        exec->spawn([&]() {
            char b = 42;
            writeEnd.write(&b, 1);
        });

        exec->join();
        STD_INSIST(result == 42);
    }

    STD_TEST(Timeout) {
        // poll on idle pipe with short timeout — must return 0
        auto exec = CoroExecutor::create(4);
        u32 pollResult = 1;
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);

        exec->spawn([&]() {
            pollResult = exec->poll(readEnd.get(), PollFlag::In, 1000);
        });

        exec->join();
        STD_INSIST(pollResult == 0);
    }

    STD_TEST(MultiPipe) {
        // N coroutines each poll their own pipe, woken in reverse order
        auto exec = CoroExecutor::create(4);
        const int N = 8;
        ScopedFD readEnds[N], writeEnds[N];
        int order[N];
        int orderIdx = 0;

        for (int i = 0; i < N; i++) {
            createPipeFD(readEnds[i], writeEnds[i]);
        }

        for (int i = 0; i < N; i++) {
            exec->spawn([&, i]() {
                u32 ready = exec->poll(readEnds[i].get(), PollFlag::In);
                STD_INSIST(ready & PollFlag::In);
                char buf;
                readEnds[i].read(&buf, 1);
                order[stdAtomicAddAndFetch(&orderIdx, 1, MemoryOrder::Relaxed) - 1] = i;
            });
        }

        // wake in reverse order
        exec->spawn([&]() {
            for (int i = N - 1; i >= 0; i--) {
                char b = 1;
                writeEnds[i].write(&b, 1);
            }
        });

        exec->join();
        STD_INSIST(orderIdx == N);
    }

    STD_TEST(SameFdMultiPoll) {
        // N coroutines all poll the same read fd; one write wakes all of them.
        // Nobody reads, so the byte stays in the pipe — late-registering coroutines
        // are still woken when they arm the fd (level-triggered ONESHOT epoll).
        auto exec = CoroExecutor::create(4);
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);
        const int N = 40;
        int woken = 0;

        for (int i = 0; i < N; i++) {
            exec->spawn([&]() {
                u32 ready = exec->poll(readEnd.get(), PollFlag::In);
                STD_INSIST(ready & PollFlag::In);
                stdAtomicAddAndFetch(&woken, 1, MemoryOrder::Relaxed);
            });
        }

        exec->spawn([&]() {
            char b = 1;
            writeEnd.write(&b, 1);
        });

        exec->join();
        STD_INSIST(woken == N);
    }

    STD_TEST(_PipeThroughput) {
        auto exec = CoroExecutor::create(8);
        const int N = 200;
        const size_t TOTAL = 500 * 1024 * 1024;

        struct Pipe {
            ScopedFD r, w;
        };

        Pipe* pipes = new Pipe[N];

        for (int i = 0; i < N; i++) {
            createPipeFD(pipes[i].r, pipes[i].w);

            pipes[i].r.setNonBlocking();
            pipes[i].w.setNonBlocking();

            int rfd = pipes[i].r.get();
            int wfd = pipes[i].w.get();

            CoroExecutor* ex = exec.mutPtr();

            exec->spawnRun(SpawnParams().setStackSize(64 * 1024).setRunable([ex, rfd, TOTAL]() {
                char buf[16 * 1024];
                size_t received = 0;

                while (received < TOTAL) {
                    ssize_t n = ::read(rfd, buf, sizeof(buf));

                    if (n > 0) {
                        received += (size_t)n;
                    } else if (errno == EAGAIN) {
                        ex->poll(rfd, PollFlag::In);
                    }
                }
            }));

            exec->spawnRun(SpawnParams().setStackSize(64 * 1024).setRunable([ex, wfd, TOTAL]() {
                char buf[16 * 1024] = {};
                size_t sent = 0;

                while (sent < TOTAL) {
                    size_t rem = TOTAL - sent;
                    ssize_t n = ::write(wfd, buf, rem < sizeof(buf) ? rem : sizeof(buf));

                    if (n > 0) {
                        sent += (size_t)n;
                    } else if (errno == EAGAIN) {
                        ex->poll(wfd, PollFlag::Out);
                    }
                }
            }));
        }

        exec->join();
        delete[] pipes;
    }
}
