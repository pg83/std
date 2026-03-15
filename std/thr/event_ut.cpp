#include "event.h"
#include "coro.h"
#include "thread.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(Event) {
    STD_TEST(SetThenWait) {
        Event ev;
        ev.set();
        ev.wait();
    }

    STD_TEST(WaitIdempotent) {
        Event ev;
        ev.set();
        ev.wait();
        ev.wait();
    }

    STD_TEST(WaitThenSet) {
        Event ev;
        bool done = false;

        ScopedThread t([&]() {
            ev.wait();
            done = true;
        });

        ev.set();
    }

    STD_TEST(MultipleWaiters) {
        Event ev;
        int counter = 0;

        ScopedThread t1([&]() { ev.wait(); stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        ScopedThread t2([&]() { ev.wait(); stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });
        ScopedThread t3([&]() { ev.wait(); stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed); });

        ev.set();
    }
}

STD_TEST_SUITE(CoroEvent) {
    STD_TEST(SetThenWait) {
        auto exec = CoroExecutor::create(4);
        Event ev(exec.mutPtr());

        exec->spawn([&]() {
            ev.set();
        });

        exec->spawn([&]() {
            ev.wait();
        });

        exec->join();
    }

    STD_TEST(WaitIdempotent) {
        auto exec = CoroExecutor::create(4);
        Event ev(exec.mutPtr());

        exec->spawn([&]() {
            ev.set();
        });

        exec->spawn([&]() {
            ev.wait();
            ev.wait();
        });

        exec->join();
    }

    STD_TEST(WaitThenSet) {
        auto exec = CoroExecutor::create(4);
        Event ev(exec.mutPtr());
        bool done = false;

        exec->spawn([&]() {
            ev.wait();
            done = true;
        });

        exec->spawn([&]() {
            ev.set();
        });

        exec->join();
        STD_INSIST(done);
    }

    STD_TEST(MultipleWaiters) {
        const int N = 8;
        auto exec = CoroExecutor::create(4);
        Event ev(exec.mutPtr());
        int counter = 0;

        for (int i = 0; i < N; ++i) {
            exec->spawn([&]() {
                ev.wait();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        exec->spawn([&]() {
            ev.set();
        });

        exec->join();
        STD_INSIST(counter == N);
    }
}
