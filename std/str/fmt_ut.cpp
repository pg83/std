#include "fmt.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(FormatU64Base10) {
    STD_TEST(Zero) {
        char buf[32];
        void* end = formatU64Base10(0, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 1);
        STD_INSIST(buf[0] == '0');
    }

    STD_TEST(SingleDigit) {
        char buf[32];
        void* end = formatU64Base10(5, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 1);
        STD_INSIST(buf[0] == '5');
    }

    STD_TEST(TwoDigits) {
        char buf[32];
        void* end = formatU64Base10(42, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 2);
        STD_INSIST(buf[0] == '4');
        STD_INSIST(buf[1] == '2');
    }

    STD_TEST(ThreeDigits) {
        char buf[32];
        void* end = formatU64Base10(123, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 3);
        STD_INSIST(buf[0] == '1');
        STD_INSIST(buf[1] == '2');
        STD_INSIST(buf[2] == '3');
    }

    STD_TEST(PowerOfTen) {
        char buf[32];
        void* end = formatU64Base10(1000, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 4);
        STD_INSIST(buf[0] == '1');
        STD_INSIST(buf[1] == '0');
        STD_INSIST(buf[2] == '0');
        STD_INSIST(buf[3] == '0');
    }

    STD_TEST(MaxU32) {
        char buf[32];
        void* end = formatU64Base10(4294967295ULL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 10);
        STD_INSIST(buf[0] == '4');
        STD_INSIST(buf[9] == '5');
    }

    STD_TEST(MaxU64) {
        char buf[32];
        void* end = formatU64Base10(18446744073709551615ULL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 20);
        STD_INSIST(buf[0] == '1');
        STD_INSIST(buf[1] == '8');
    }

    STD_TEST(LargeNumber) {
        char buf[32];
        void* end = formatU64Base10(1234567890123456789ULL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 19);
        STD_INSIST(buf[0] == '1');
        STD_INSIST(buf[18] == '9');
    }

    STD_TEST(TrailingZeros) {
        char buf[32];
        void* end = formatU64Base10(1000000, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 7);
        STD_INSIST(buf[0] == '1');
        for (size_t i = 1; i < 7; ++i) {
            STD_INSIST(buf[i] == '0');
        }
    }

    STD_TEST(AllNines) {
        char buf[32];
        void* end = formatU64Base10(999, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 3);
        STD_INSIST(buf[0] == '9');
        STD_INSIST(buf[1] == '9');
        STD_INSIST(buf[2] == '9');
    }
}

#define SUITE_NAME SUITE_NAME_2

STD_TEST_SUITE(FormatI64Base10) {
    STD_TEST(Zero) {
        char buf[32];
        void* end = formatI64Base10(0, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 1);
        STD_INSIST(buf[0] == '0');
    }

    STD_TEST(PositiveSingleDigit) {
        char buf[32];
        void* end = formatI64Base10(7, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 1);
        STD_INSIST(buf[0] == '7');
    }

    STD_TEST(NegativeSingleDigit) {
        char buf[32];
        void* end = formatI64Base10(-3, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 2);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '3');
    }

    STD_TEST(PositiveTwoDigits) {
        char buf[32];
        void* end = formatI64Base10(42, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 2);
        STD_INSIST(buf[0] == '4');
        STD_INSIST(buf[1] == '2');
    }

    STD_TEST(NegativeTwoDigits) {
        char buf[32];
        void* end = formatI64Base10(-42, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 3);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '4');
        STD_INSIST(buf[2] == '2');
    }

    STD_TEST(PositiveThreeDigits) {
        char buf[32];
        void* end = formatI64Base10(123, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 3);
        STD_INSIST(buf[0] == '1');
        STD_INSIST(buf[1] == '2');
        STD_INSIST(buf[2] == '3');
    }

    STD_TEST(NegativeThreeDigits) {
        char buf[32];
        void* end = formatI64Base10(-123, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 4);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '1');
        STD_INSIST(buf[2] == '2');
        STD_INSIST(buf[3] == '3');
    }

    STD_TEST(MaxI32) {
        char buf[32];
        void* end = formatI64Base10(2147483647LL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 10);
        STD_INSIST(buf[0] == '2');
        STD_INSIST(buf[9] == '7');
    }

    STD_TEST(MinI32) {
        char buf[32];
        void* end = formatI64Base10(-2147483648LL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 11);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '2');
    }

    STD_TEST(MaxI64) {
        char buf[32];
        void* end = formatI64Base10(9223372036854775807LL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 19);
        STD_INSIST(buf[0] == '9');
        STD_INSIST(buf[18] == '7');
    }

    STD_TEST(MinI64) {
        char buf[32];
        void* end = formatI64Base10(-9223372036854775807LL - 1, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 20);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '9');
    }

    STD_TEST(NegativePowerOfTen) {
        char buf[32];
        void* end = formatI64Base10(-1000, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 5);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '1');
        STD_INSIST(buf[2] == '0');
        STD_INSIST(buf[3] == '0');
        STD_INSIST(buf[4] == '0');
    }

    STD_TEST(PositiveLargeNumber) {
        char buf[32];
        void* end = formatI64Base10(1234567890123456789LL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 19);
        STD_INSIST(buf[0] == '1');
        STD_INSIST(buf[18] == '9');
    }

    STD_TEST(NegativeLargeNumber) {
        char buf[32];
        void* end = formatI64Base10(-1234567890123456789LL, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 20);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '1');
        STD_INSIST(buf[19] == '9');
    }

    STD_TEST(MinusOne) {
        char buf[32];
        void* end = formatI64Base10(-1, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 2);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '1');
    }

    STD_TEST(NegativeTrailingZeros) {
        char buf[32];
        void* end = formatI64Base10(-1000000, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 8);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '1');
        for (size_t i = 2; i < 8; ++i) {
            STD_INSIST(buf[i] == '0');
        }
    }

    STD_TEST(NegativeAllNines) {
        char buf[32];
        void* end = formatI64Base10(-999, buf);
        size_t len = (char*)end - buf;
        STD_INSIST(len == 4);
        STD_INSIST(buf[0] == '-');
        STD_INSIST(buf[1] == '9');
        STD_INSIST(buf[2] == '9');
        STD_INSIST(buf[3] == '9');
    }
}
