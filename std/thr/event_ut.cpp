#include "event.h"
#include "coro.h"
#include "thread.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

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
}

STD_TEST_SUITE(EventCoro) {
    STD_TEST(BasicSignal) {
        auto exec = CoroExecutor::create(4);

        exec->spawn([&] {
            Event ev(exec.mutPtr());
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
        auto exec = CoroExecutor::create(4);
        const int N = 100;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                Event ev(exec.mutPtr());
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
        auto exec = CoroExecutor::create(4);
        const int N = 8;
        int counter = 0;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                exec->spawn([&] {
                    Event ev(exec.mutPtr());
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
