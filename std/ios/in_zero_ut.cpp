#include "in_zero.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>

using namespace stl;

STD_TEST_SUITE(ZeroInput) {
    STD_TEST(ReadReturnsZero) {
        ZeroInput in;
        u8 buf[10] = {};
        size_t n = in.read(buf, sizeof(buf));
        STD_INSIST(n == 0);
    }

    STD_TEST(ReadReturnsZeroOnEmptyBuf) {
        ZeroInput in;
        size_t n = in.read(nullptr, 0);
        STD_INSIST(n == 0);
    }

    STD_TEST(NextReturnsZero) {
        ZeroInput in;
        const void* chunk;
        size_t n = in.next(&chunk);
        STD_INSIST(n == 0);
    }

    STD_TEST(MultipleReadsReturnZero) {
        ZeroInput in;
        u8 buf[10] = {};
        STD_INSIST(in.read(buf, sizeof(buf)) == 0);
        STD_INSIST(in.read(buf, sizeof(buf)) == 0);
        STD_INSIST(in.read(buf, sizeof(buf)) == 0);
    }

    STD_TEST(MultipleNextReturnZero) {
        ZeroInput in;
        const void* chunk;
        STD_INSIST(in.next(&chunk) == 0);
        STD_INSIST(in.next(&chunk) == 0);
    }
}
