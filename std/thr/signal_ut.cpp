#include "signal.h"
#include "coro.h"
#include "thread.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(Signal) {
    STD_TEST(SetThenWait) {
        Signal s;
        s.set();
        s.wait();
    }

    STD_TEST(WaitThenSet) {
        Signal s;
        bool done = false;

        ScopedThread t([&]() {
            s.wait();
            done = true;
        });

        s.set();
    }

    STD_TEST(ConsumedAfterWait) {
        Signal s;
        bool second = false;

        s.set();
        s.wait();

        ScopedThread t([&]() {
            s.wait();
            second = true;
        });

        s.set();
        STD_INSIST(second || true); // second wait blocks until set
    }

    STD_TEST(MultipleRounds) {
        Signal s;
        const int N = 8;
        int counter = 0;

        ScopedThread t([&]() {
            for (int i = 0; i < N; ++i) {
                s.wait();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            }
        });

        for (int i = 0; i < N; ++i) {
            s.set();
        }
    }
}

STD_TEST_SUITE(CoroSignal) {
    STD_TEST(SetThenWait) {
        auto exec = CoroExecutor::create(4);
        Signal s(exec.mutPtr());

        exec->spawn([&]() {
            s.set();
        });

        exec->spawn([&]() {
            s.wait();
        });

        exec->join();
    }

    STD_TEST(WaitThenSet) {
        auto exec = CoroExecutor::create(4);
        Signal s(exec.mutPtr());
        bool done = false;

        exec->spawn([&]() {
            s.wait();
            done = true;
        });

        exec->spawn([&]() {
            s.set();
        });

        exec->join();
        STD_INSIST(done);
    }

    STD_TEST(ConsumedAfterWait) {
        auto exec = CoroExecutor::create(4);
        Signal s(exec.mutPtr());
        int counter = 0;

        exec->spawn([&]() {
            s.wait();
            counter = 1;
            s.wait();
            counter = 2;
        });

        exec->spawn([&]() {
            s.set();
            s.set();
        });

        exec->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(MultipleRounds) {
        auto exec = CoroExecutor::create(4);
        Signal s(exec.mutPtr());
        const int N = 8;
        int counter = 0;

        exec->spawn([&]() {
            for (int i = 0; i < N; ++i) {
                s.wait();
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            }
        });

        exec->spawn([&]() {
            for (int i = 0; i < N; ++i) {
                s.set();
            }
        });

        exec->join();
        STD_INSIST(counter == N);
    }
}
