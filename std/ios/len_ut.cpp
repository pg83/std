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

    STD_TEST(testWriteV) {
        CountingOutput out;
        StringView parts[] = {
            StringView(u8"Hello"),
            StringView(u8" "),
            StringView(u8"World"),
            StringView(u8"!"),
        };

        out.writeV(parts, 4);

        STD_INSIST(out.collectedLength() == 12);
    }

    STD_TEST(testWriteVEmpty) {
        CountingOutput out;
        StringView parts[] = {
            StringView(u8""),
            StringView(u8"test"),
            StringView(u8""),
        };

        out.writeV(parts, 3);

        STD_INSIST(out.collectedLength() == 4);
    }

    STD_TEST(testWriteVMultipleCalls) {
        CountingOutput out;
        StringView parts1[] = {
            StringView(u8"First"),
            StringView(u8" "),
            StringView(u8"call"),
        };
        StringView parts2[] = {
            StringView(u8"Second"),
            StringView(u8" "),
            StringView(u8"call"),
        };

        out.writeV(parts1, 3);
        out.writeV(parts2, 3);

        STD_INSIST(out.collectedLength() == 21);
    }
}
