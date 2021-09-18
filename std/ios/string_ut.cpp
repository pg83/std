#include "string.h"

#include <std/tst/ut.h>
#include <std/str/dynamic.h>

using namespace Std;

STD_TEST_SUITE(ioString) {
    STD_TEST(write) {
        DynString str;

        {
            StringOutput out(str);

            out.write(u8"1", 1);
            out.write(u8"23", 2);
            out.write(u8"4567", 4);
            out.write(u8"89101112", 8);
        }

        STD_INSIST(str == StringView(u8"123456789101112"));
    }
}
