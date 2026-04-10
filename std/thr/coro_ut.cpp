#include "coro.h"

#include "pool.h"
#include "guard.h"
#include "mutex.h"
#include "async.h"
#include "channel.h"
#include "cond_var.h"
#include "semaphore.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace stl;

STD_TEST_SUITE(CoroExecutor) {
    STD_TEST(Basic) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [] {
            return 1;
        });

        STD_INSIST(f.wait() == 1);
    }

    STD_TEST(SingleYield) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [&] {
            int counter = 0;
            ++counter;
            exec->yield();
            ++counter;
            return counter;
        });

        STD_INSIST(f.wait() == 2);
    }

    STD_TEST(MultipleYields) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [&] {
            int counter = 0;
            for (int i = 0; i < 10; ++i) {
                ++counter;
                exec->yield();
            }
            return counter;
        });

        STD_INSIST(f.wait() == 10);
    }

    STD_TEST(ManyCoros) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&] {
                exec->yield();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        exec->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(NestedSpawn) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;

        exec->spawn([&] {
            exec->spawn([&] {
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });

            stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
        });

        exec->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(YieldInterleave) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto fn = [&] {
            for (int i = 0; i < 5; ++i) {
                exec->yield();
            }
        };

        exec->spawn(fn);
        exec->spawn(fn);

        exec->join();
    }

    STD_TEST(MutexBasic) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [&] {
            Mutex mtx(exec);

            mtx.lock();
            mtx.unlock();
            return 1;
        });

        STD_INSIST(f.wait() == 1);
    }

    STD_TEST(MutexContention) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        Mutex mtx(exec);

        for (int i = 0; i < 2; ++i) {
            exec->spawn([&] {
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int nCoros = 8;
        const int nIters = 200;
        int counter = 0;
        Mutex mtx(exec);

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&] {
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int value = 0;
        Mutex mtx(exec);
        CondVar cv(exec);

        exec->spawn([&] {
            LockGuard guard(mtx);
            while (value == 0) {
                cv.wait(mtx);
            }
        });

        exec->spawn([&] {
            LockGuard guard(mtx);
            value = 1;
            cv.signal();
        });

        exec->join();
        STD_INSIST(value == 1);
    }

    STD_TEST(CondVarBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 5;
        int value = 0;
        Mutex mtx(exec);
        CondVar cv(exec);

        for (int i = 0; i < N; ++i) {
            exec->spawn([&] {
                LockGuard guard(mtx);
                while (value == 0) {
                    cv.wait(mtx);
                }
            });
        }

        exec->spawn([&] {
            LockGuard guard(mtx);
            value = 1;
            cv.broadcast();
        });

        exec->join();
        STD_INSIST(value == 1);
    }

    STD_TEST(CondVarProducerConsumer) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 10;
        int produced = 0;
        int consumed = 0;
        Mutex mtx(exec);
        CondVar cv(exec);

        // consumer
        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                LockGuard guard(mtx);
                while (produced == consumed) {
                    cv.wait(mtx);
                }
                ++consumed;
            }
        });

        // producer
        exec->spawn([&] {
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int nCoros = 4;
        const int nIters = 20;
        int queue = 0;
        int consumed = 0;
        Mutex mtx(exec);
        CondVar cv(exec);

        for (int i = 0; i < nCoros; ++i) {
            exec->spawn([&] {
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

        exec->spawn([&] {
            for (int j = 0; j < nCoros * nIters; ++j) {
                LockGuard guard(mtx);
                ++queue;
                cv.signal();
            }
        });

        exec->join();
        STD_INSIST(consumed == nCoros * nIters);
    }
}

STD_TEST_SUITE(CoroRandom) {
    STD_TEST(NonZero) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [&] {
            return exec->random();
        });

        STD_INSIST(f.wait() != 0);
    }

    STD_TEST(DifferentPerCoro) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 8;
        u32 values[N] = {};

        for (int i = 0; i < N; ++i) {
            exec->spawn([&, i] {
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        struct Pair {
            u32 a, b;
        };

        auto f = async(exec, [&] {
            return Pair{exec->random(), exec->random()};
        });

        auto& p = f.wait();
        STD_INSIST(p.a != p.b);
    }
}

STD_TEST_SUITE(CoroMisc) {
    STD_TEST(SpawnFromMain) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;
        Mutex mtx(exec);

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&] {
                LockGuard guard(mtx);
                ++counter;
            });
        }

        exec->join();
        STD_INSIST(counter == 100);
    }
}

STD_TEST_SUITE(CoroOffload) {
    STD_TEST(Basic) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 4);
        auto pool = ThreadPool::simple(opool.mutPtr(), 2);

        auto f = async(exec, [&] {
            int result = 0;

            exec->offload(pool, [&] {
                result = 42;
            });

            return result;
        });

        STD_INSIST(f.wait() == 42);
    }

    STD_TEST(MultipleCalls) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 4);
        auto pool = ThreadPool::simple(opool.mutPtr(), 2);

        auto f = async(exec, [&] {
            int sum = 0;

            for (int i = 0; i < 10; ++i) {
                exec->offload(pool, [&] {
                    sum += 1;
                });
            }

            return sum;
        });

        STD_INSIST(f.wait() == 10);
    }

    STD_TEST(ConcurrentCoros) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 4);
        auto pool = ThreadPool::simple(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 16; ++i) {
            exec->spawn([&] {
                exec->offload(pool, [&] {
                    stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
                });
            });
        }

        exec->join();
        STD_INSIST(counter == 16);
    }

    STD_TEST(HeavyWork) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 4);
        auto pool = ThreadPool::simple(opool.mutPtr(), 2);

        auto f = async(exec, [&] {
            int result = 0;

            exec->offload(pool, [&] {
                for (volatile int i = 0; i < 10000; i = i + 1) {
                }
                result = 1;
            });

            return result;
        });

        STD_INSIST(f.wait() == 1);
    }
}
