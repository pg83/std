#include "num_cpu.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>

#include <stdlib.h>

using namespace stl;

STD_TEST_SUITE(NumCpu) {
    STD_TEST(ReturnsPositive) {
        STD_INSIST(numCpu() > 0);
    }

    STD_TEST(GomaxprocsOverride) {
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
        auto* prev = getenv("GOMAXPROCS");

        unsetenv("GOMAXPROCS");

        auto n = numCpu();

        STD_INSIST(n > 0);

        if (prev) {
            setenv("GOMAXPROCS", prev, 1);
        }
    }
}
