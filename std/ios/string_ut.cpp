#include "buf.h"
#include "string.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>
#include <std/str/dynamic.h>

using namespace Std;

STD_TEST_SUITE(StringIO) {
    STD_TEST(write) {
        DynString str;

        {
            StringOutput out(str);

            out.write(u8"1", 1);
            out.write(u8"23", 2);
            out.write(u8"4567", 4);
            out.write(u8"89101112", 8);
        }

        STD_INSIST(StringView(str) == StringView(u8"123456789101112"));
    }

    struct String: public DynString, public StringOutput {
        inline String() noexcept
            : StringOutput(*(DynString*)this)
        {
        }
    };

    STD_TEST(numbers) {
        STD_INSIST(StringView(String() << 1 << -2 << finI) == StringView(u8"1-2"));
    }
}
