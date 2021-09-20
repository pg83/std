#include "buffer.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Buffer) {
    STD_TEST(reset) {
        Buffer b;

        b.reset();

        STD_INSIST(b.empty());
    }
}
