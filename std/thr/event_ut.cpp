#include "event.h"
#include "coro.h"
#include "mutex.h"
#include "thread.h"
#include "coro_config.h"

#include <std/tst/ut.h>
#include <std/alg/range.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(EventDefault) {
    STD_TEST(SignalThenWait) {
        Event ev;

        ev.signal();
        ev.wait(makeRunable([] {}));
    }

    STD_TEST(WaitOnThread) {
        Event ev;
        int value = 0;

        ScopedThread t([&] {
            stdAtomicStore(&value, 1, MemoryOrder::Release);
            ev.signal();
        });

        ev.wait(makeRunable([] {}));
        STD_INSIST(stdAtomicFetch(&value, MemoryOrder::Acquire) == 1);
    }

    STD_TEST(CallbackRunsBeforeBlock) {
        Event ev;
        int order = 0;

        {
            ScopedThread t([&] {
                while (stdAtomicFetch(&order, MemoryOrder::Acquire) < 1) {
                }

                ev.signal();
            });

            ev.wait(makeRunable([&] {
                stdAtomicStore(&order, 1, MemoryOrder::Release);
            }));
        }

        STD_INSIST(order == 1);
    }

    STD_TEST(CallbackUnlocksMutex) {
        Event ev;
        Mutex mu;
        int value = 0;

        mu.lock();

        {
            ScopedThread t([&] {
                mu.lock();
                value = 42;
                mu.unlock();
                ev.signal();
            });

            ev.wait(makeRunable([&] {
                mu.unlock();
            }));
        }

        STD_INSIST(value == 42);
    }

    STD_TEST(StressManyEvents) {
        const int N = 20;

        for (int i = 0; i < N; ++i) {
            Event ev;
            int value = 0;

            {
                ScopedThread t([&] {
                    value = i + 1;
                    ev.signal();
                });

                ev.wait(makeRunable([] {}));
            }

            STD_INSIST(value == i + 1);
        }
    }

    STD_TEST(StressSignalBeforeWait) {
        const int N = 20;

        for (int i = 0; i < N; ++i) {
            Event ev;

            ev.signal();
            ev.wait(makeRunable([] {}));
        }
    }

    STD_TEST(StressConcurrentPairs) {
        const int N = 5;
        const int THREADS = 4;
        u32 counter = 0;

        auto pool = ObjPool::fromMemory();
        Vector<Thread*> threads;

        for (int j = 0; j < THREADS; ++j) {
            auto r = makeRunablePtr([&] {
                for (int i = 0; i < N; ++i) {
                    Event ev;
                    int v = 0;

                    {
                        ScopedThread t([&] {
                            v = 1;
                            ev.signal();
                        });

                        ev.wait(makeRunable([] {}));
                    }

                    stdAtomicAddAndFetch(&counter, (u32)v, MemoryOrder::Relaxed);
                }
            });

            threads.pushBack(pool->make<Thread>(*r));
        }

        for (auto t : range(threads)) {
            t->join();
        }

        STD_INSIST(counter == N * THREADS);
    }

    STD_TEST(StressDirectHandoff) {
        const int N = 10;

        for (int i = 0; i < N; ++i) {
            Event ev;
            Mutex mu;
            int slot = 0;

            mu.lock();

            {
                ScopedThread t([&] {
                    mu.lock();
                    slot = i + 1;
                    mu.unlock();
                    ev.signal();
                });

                ev.wait(makeRunable([&] {
                    mu.unlock();
                }));
            }

            STD_INSIST(slot == i + 1);
        }
    }
}

STD_TEST_SUITE(EventCoro) {
    STD_TEST(BasicSignal) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        exec->spawn([&] {
            Event ev(exec);
            int value = 0;

            exec->spawn([&] {
                value = 42;
                ev.signal();
            });

            ev.wait(makeRunable([] {}));
            STD_INSIST(value == 42);
        });

        exec->join();
    }

    STD_TEST(DirectHandoff) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 100;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                Event ev(exec);
                int result = 0;

                exec->spawn([&] {
                    result = i + 1;
                    ev.signal();
                });

                ev.wait(makeRunable([] {}));
                STD_INSIST(result == i + 1);
            }
        });

        exec->join();
    }

    STD_TEST(ManyPairs) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 8;
        int counter = 0;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                exec->spawn([&] {
                    Event ev(exec);
                    int val = 0;

                    exec->spawn([&] {
                        val = 1;
                        ev.signal();
                    });

                    ev.wait(makeRunable([] {}));
                    stdAtomicAddAndFetch(&counter, val, MemoryOrder::Relaxed);
                });
            }

            while (stdAtomicFetch(&counter, MemoryOrder::Acquire) < N) {
                exec->yield();
            }

            STD_INSIST(counter == N);
        });

        exec->join();
    }
}
