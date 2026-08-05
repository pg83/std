#include "num.h"

#include "view.h"

#include <std/tst/ut.h>

using namespace stl;

STD_TEST_SUITE(Num) {
    STD_TEST(SignedParse) {
        i64 value = 7;

        STD_INSIST(parseI64(StringView(u8"0"), value) && value == 0);
        STD_INSIST(parseI64(StringView(u8"42"), value) && value == 42);
        STD_INSIST(parseI64(StringView(u8"-42"), value) && value == -42);
        STD_INSIST(parseI64(StringView(u8"+42"), value) && value == 42);
        STD_INSIST(parseI64(StringView(u8"9223372036854775807"), value) && value == 9223372036854775807ll);
        STD_INSIST(parseI64(StringView(u8"-9223372036854775808"), value) && value == (-9223372036854775807ll - 1));
        STD_INSIST(!parseI64(StringView(u8"9223372036854775808"), value));
        STD_INSIST(!parseI64(StringView(), value));
        STD_INSIST(!parseI64(StringView(u8"-"), value));
        STD_INSIST(!parseI64(StringView(u8"12x"), value));
        STD_INSIST(!parseI64(StringView(u8" 12"), value));
    }

    STD_TEST(UnsignedParse) {
        u64 value = 7;

        STD_INSIST(parseU64(StringView(u8"18446744073709551615"), value) && value == ~(u64)(0));
        STD_INSIST(!parseU64(StringView(u8"18446744073709551616"), value));
        STD_INSIST(parseU64(StringView(u8"ff"), value, 16) && value == 0xff);
        STD_INSIST(parseU64(StringView(u8"777"), value, 8) && value == 0777);
        STD_INSIST(parseU64(StringView(u8"101"), value, 2) && value == 5);
        STD_INSIST(!parseU64(StringView(u8"-1"), value));
        STD_INSIST(!parseU64(StringView(u8"8"), value, 8));
    }

    STD_TEST(FloatParse) {
        double value = 7.0;

        STD_INSIST(parseF64(StringView(u8"1.5"), value) && value == 1.5);
        STD_INSIST(parseF64(StringView(u8"-2.5e2"), value) && value == -250.0);
        STD_INSIST(parseF64(StringView(u8"0"), value) && value == 0.0);
        STD_INSIST(!parseF64(StringView(u8"1.5x"), value));
        STD_INSIST(!parseF64(StringView(), value));
    }
}
