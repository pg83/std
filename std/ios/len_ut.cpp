#include "len.h"

#include <std/tst/ut.h>

#include <string.h>

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

    STD_TEST(testImbueSmallBuffer) {
        CountingOutput out;
        auto buf = out.imbue(100);
        STD_INSIST(buf.ptr != nullptr);

        const char* data = "Hello";
        memcpy(buf.ptr, data, 5);
        out.bump((u8*)buf.ptr + 5);

        STD_INSIST(out.collectedLength() == 5);
    }

    STD_TEST(testImbueLargeBuffer) {
        CountingOutput out;
        auto buf = out.imbue(2048);
        STD_INSIST(buf.ptr != nullptr);

        memset(buf.ptr, 'X', 2048);
        out.bump((u8*)buf.ptr + 2048);

        STD_INSIST(out.collectedLength() == 2048);
    }

    STD_TEST(testImbueMultipleSmall) {
        CountingOutput out;

        for (size_t i = 0; i < 10; ++i) {
            auto buf = out.imbue(50);
            memset(buf.ptr, 'A', 50);
            out.bump((u8*)buf.ptr + 50);
        }

        STD_INSIST(out.collectedLength() == 500);
    }

    STD_TEST(testImbueMultipleLarge) {
        CountingOutput out;

        for (size_t i = 0; i < 5; ++i) {
            auto buf = out.imbue(1500);
            memset(buf.ptr, 'B', 1500);
            out.bump((u8*)buf.ptr + 1500);
        }

        STD_INSIST(out.collectedLength() == 7500);
    }

    STD_TEST(testImbueMixedSizes) {
        CountingOutput out;

        auto small = out.imbue(100);
        out.bump((u8*)small.ptr + 100);

        auto large = out.imbue(2000);
        out.bump((u8*)large.ptr + 2000);

        auto small2 = out.imbue(50);
        out.bump((u8*)small2.ptr + 50);

        STD_INSIST(out.collectedLength() == 2150);
    }

    STD_TEST(testImbueZeroBytes) {
        CountingOutput out;

        auto buf = out.imbue(100);
        out.bump(buf.ptr);

        STD_INSIST(out.collectedLength() == 0);
    }

    STD_TEST(testImbuePartialUse) {
        CountingOutput out;

        auto buf = out.imbue(1000);
        out.bump((u8*)buf.ptr + 250);

        STD_INSIST(out.collectedLength() == 250);
    }

    STD_TEST(testMixedWriteAndImbue) {
        CountingOutput out;

        out << StringView(u8"Hello");

        auto buf = out.imbue(100);
        memcpy(buf.ptr, " World", 6);
        out.bump((u8*)buf.ptr + 6);

        out << StringView(u8"!");

        STD_INSIST(out.collectedLength() == 12);
    }

    STD_TEST(testHint) {
        CountingOutput out;
        STD_INSIST(out.hint() == 1024);
    }

    STD_TEST(testInitialLength) {
        CountingOutput out;
        STD_INSIST(out.collectedLength() == 0);
    }

    STD_TEST(testImbueAtBoundary) {
        CountingOutput out;

        auto buf1 = out.imbue(1024);
        out.bump((u8*)buf1.ptr + 1024);

        auto buf2 = out.imbue(1025);
        out.bump((u8*)buf2.ptr + 1025);

        STD_INSIST(out.collectedLength() == 2049);
    }

    STD_TEST(testMultipleWriteMethods) {
        CountingOutput out;

        out.write(u8"abc", 3);

        StringView parts[] = {
            StringView(u8"def"),
            StringView(u8"ghi"),
        };
        out.writeV(parts, 2);

        auto buf = out.imbue(200);
        memcpy(buf.ptr, "jkl", 3);
        out.bump((u8*)buf.ptr + 3);

        out << StringView(u8"mno");

        STD_INSIST(out.collectedLength() == 15);
    }
}
