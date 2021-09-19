#include "arc.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(ARC) {
    STD_TEST(test) {
        ARC rc;

        STD_INSIST(rc.refCount() == 0);
        STD_INSIST(rc.ref() == 1);
        STD_INSIST(rc.refCount() == 1);
        STD_INSIST(rc.unref() == 0);
        STD_INSIST(rc.refCount() == 0);
    }
}
