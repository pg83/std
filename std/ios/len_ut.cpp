#include "len.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(CountingOutput) {
    STD_TEST(testSimple) {
        STD_INSIST((CountingOutput() << StringView(u8"12345") << 12345).collectedLength() == 10);
    }

    STD_TEST(testBig) {
        CountingOutput out;

        for (size_t i = 0; i < 10000; ++i) {
            out << 12345;
        }

        STD_INSIST(out.collectedLength() == 50000);
    }
}
