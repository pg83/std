#include "ut.h"

#include <std/ios/output.h>

using namespace Std;

STD_TEST_SUITE(ut) {
    STD_TEST(test1) {
        stdE << StringView(u8"here") << endL;
    }
}
