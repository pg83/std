#include "view.h"
#include "builder.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>
#include <std/str/dynamic.h>

using namespace Std;

STD_TEST_SUITE(StringBuilder) {
    STD_TEST(numbers) {
        STD_INSIST(StringView(StringBuilder() << 1 << -2 << finI) == StringView(u8"1-2"));
    }
}
