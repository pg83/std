#include "num_cpu.h"

#include <std/tst/ut.h>
#include <std/thr/mutex.h>
#include <std/thr/guard.h>
#include <std/dbg/insist.h>

#include <stdlib.h>

using namespace stl;

namespace {
    Mutex& envMutex() {
        static Mutex m;

        return m;
    }
}

STD_TEST_SUITE(NumCpu) {
    STD_TEST(ReturnsPositive) {
        LockGuard g(envMutex());

        STD_INSIST(numCpu() > 0);
    }

    STD_TEST(GomaxprocsOverride) {
        LockGuard g(envMutex());

        auto* prev = getenv("GOMAXPROCS");

        setenv("GOMAXPROCS", "7", 1);
        STD_INSIST(numCpu() == 7);

        setenv("GOMAXPROCS", "1", 1);
        STD_INSIST(numCpu() == 1);

        if (prev) {
            setenv("GOMAXPROCS", prev, 1);
        } else {
            unsetenv("GOMAXPROCS");
        }
    }

    STD_TEST(GomaxprocsZeroIgnored) {
        LockGuard g(envMutex());

        auto* prev = getenv("GOMAXPROCS");

        setenv("GOMAXPROCS", "0", 1);
        STD_INSIST(numCpu() > 0);

        if (prev) {
            setenv("GOMAXPROCS", prev, 1);
        } else {
            unsetenv("GOMAXPROCS");
        }
    }

    STD_TEST(GomaxprocsGarbageIgnored) {
        LockGuard g(envMutex());

        auto* prev = getenv("GOMAXPROCS");

        setenv("GOMAXPROCS", "abc", 1);
        STD_INSIST(numCpu() > 0);

        if (prev) {
            setenv("GOMAXPROCS", prev, 1);
        } else {
            unsetenv("GOMAXPROCS");
        }
    }

    STD_TEST(WithoutEnvMatchesOs) {
        LockGuard g(envMutex());

        auto* prev = getenv("GOMAXPROCS");

        unsetenv("GOMAXPROCS");

        auto n = numCpu();

        STD_INSIST(n > 0);

        if (prev) {
            setenv("GOMAXPROCS", prev, 1);
        }
    }
}
