#include "in_zc.h"
#include "in_mem.h"

#include <std/tst/ut.h>
#include <std/lib/buffer.h>

#include <cstring>

using namespace Std;

STD_TEST_SUITE(ZeroCopyInputReadLine) {
    STD_TEST(EmptyInput) {
        const char* data = "";
        MemoryInput input(data, 0);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.empty());
    }

    STD_TEST(SingleLineWithNewline) {
        const char* data = "hello\n";
        MemoryInput input(data, 6);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 5);
        STD_INSIST(memcmp(buf.data(), "hello", 5) == 0);
    }

    STD_TEST(SingleLineWithoutNewline) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 5);
        STD_INSIST(memcmp(buf.data(), "hello", 5) == 0);
    }

    STD_TEST(EmptyLineWithNewline) {
        const char* data = "\n";
        MemoryInput input(data, 1);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.empty());
    }

    STD_TEST(MultipleLines) {
        const char* data = "first\nsecond\nthird";
        MemoryInput input(data, 18);
        Buffer buf1, buf2, buf3;
        input.readLine(buf1);
        input.readLine(buf2);
        input.readLine(buf3);
        STD_INSIST(buf1.length() == 5);
        STD_INSIST(memcmp(buf1.data(), "first", 5) == 0);
        STD_INSIST(buf2.length() == 6);
        STD_INSIST(memcmp(buf2.data(), "second", 6) == 0);
        STD_INSIST(buf3.length() == 5);
        STD_INSIST(memcmp(buf3.data(), "third", 5) == 0);
    }

    STD_TEST(NewlineAtStart) {
        const char* data = "\nhello";
        MemoryInput input(data, 6);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.empty());
    }

    STD_TEST(MultipleNewlines) {
        const char* data = "line1\n\n\nline2";
        MemoryInput input(data, 13);
        Buffer buf1, buf2, buf3, buf4;
        input.readLine(buf1);
        input.readLine(buf2);
        input.readLine(buf3);
        input.readLine(buf4);
        STD_INSIST(buf1.length() == 5);
        STD_INSIST(memcmp(buf1.data(), "line1", 5) == 0);
        STD_INSIST(buf2.empty());
        STD_INSIST(buf3.empty());
        STD_INSIST(buf4.length() == 5);
        STD_INSIST(memcmp(buf4.data(), "line2", 5) == 0);
    }

    STD_TEST(LongLine) {
        const size_t lineLen = 10000;
        u8* data = new u8[lineLen + 1];
        for (size_t i = 0; i < lineLen; ++i) {
            data[i] = 'A' + (i % 26);
        }
        data[lineLen] = '\n';
        MemoryInput input(data, lineLen + 1);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == lineLen);
        for (size_t i = 0; i < lineLen; ++i) {
            STD_INSIST(((u8*)buf.data())[i] == 'A' + (i % 26));
        }
        delete[] data;
    }

    STD_TEST(BinaryDataWithNewline) {
        u8 data[] = {0, 1, 2, 255, 254, '\n', 128, 127};
        MemoryInput input(data, sizeof(data));
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 5);
        STD_INSIST(memcmp(buf.data(), data, 5) == 0);
    }

    STD_TEST(OnlyNewlines) {
        const char* data = "\n\n\n";
        MemoryInput input(data, 3);
        Buffer buf1, buf2, buf3;
        input.readLine(buf1);
        input.readLine(buf2);
        input.readLine(buf3);
        STD_INSIST(buf1.empty());
        STD_INSIST(buf2.empty());
        STD_INSIST(buf3.empty());
    }

    STD_TEST(ReadLineAfterExhaustion) {
        const char* data = "line\n";
        MemoryInput input(data, 5);
        Buffer buf1, buf2;
        input.readLine(buf1);
        input.readLine(buf2);
        STD_INSIST(buf1.length() == 4);
        STD_INSIST(memcmp(buf1.data(), "line", 4) == 0);
        STD_INSIST(buf2.empty());
    }

    STD_TEST(SingleCharLine) {
        const char* data = "a\n";
        MemoryInput input(data, 2);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 1);
        STD_INSIST(memcmp(buf.data(), "a", 1) == 0);
    }

    STD_TEST(LineWithSpaces) {
        const char* data = "  hello world  \n";
        MemoryInput input(data, 16);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 15);
        STD_INSIST(memcmp(buf.data(), "  hello world  ", 15) == 0);
    }

    STD_TEST(LinewithTabs) {
        const char* data = "\thello\t\n";
        MemoryInput input(data, 8);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 7);
        STD_INSIST(memcmp(buf.data(), "\thello\t", 7) == 0);
    }

    STD_TEST(MixedLineEndings) {
        const char* data = "line1\nline2";
        MemoryInput input(data, 11);
        Buffer buf1, buf2;
        input.readLine(buf1);
        input.readLine(buf2);
        STD_INSIST(buf1.length() == 5);
        STD_INSIST(memcmp(buf1.data(), "line1", 5) == 0);
        STD_INSIST(buf2.length() == 5);
        STD_INSIST(memcmp(buf2.data(), "line2", 5) == 0);
    }

    STD_TEST(ReuseBuffer) {
        const char* data = "line1\nline2\n";
        MemoryInput input(data, 12);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 5);
        STD_INSIST(memcmp(buf.data(), "line1", 5) == 0);
        buf.reset();
        input.readLine(buf);
        STD_INSIST(buf.length() == 5);
        STD_INSIST(memcmp(buf.data(), "line2", 5) == 0);
    }

    STD_TEST(NewlineAtEnd) {
        const char* data = "hello\n";
        MemoryInput input(data, 6);
        Buffer buf1, buf2;
        input.readLine(buf1);
        STD_INSIST(buf1.length() == 5);
        STD_INSIST(memcmp(buf1.data(), "hello", 5) == 0);
        input.readLine(buf2);
        STD_INSIST(buf2.empty());
    }

    STD_TEST(UnicodeText) {
        const char* data = "Привет мир\n";
        const size_t len = strlen(data);
        MemoryInput input(data, len);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == len - 1);
        STD_INSIST(memcmp(buf.data(), data, len - 1) == 0);
    }

    STD_TEST(VeryLongLineWithoutNewline) {
        const size_t lineLen = 50000;
        u8* data = new u8[lineLen];
        for (size_t i = 0; i < lineLen; ++i) {
            data[i] = 'X';
        }
        MemoryInput input(data, lineLen);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == lineLen);
        for (size_t i = 0; i < lineLen; ++i) {
            STD_INSIST(((u8*)buf.data())[i] == 'X');
        }
        delete[] data;
    }

    STD_TEST(CarriageReturnInLine) {
        const char* data = "hello\rworld\n";
        MemoryInput input(data, 12);
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 11);
        STD_INSIST(memcmp(buf.data(), "hello\rworld", 11) == 0);
    }

    STD_TEST(NullByteInLine) {
        u8 data[] = {'h', 'e', 'l', 'l', 'o', 0, 'w', 'o', 'r', 'l', 'd', '\n'};
        MemoryInput input(data, sizeof(data));
        Buffer buf;
        input.readLine(buf);
        STD_INSIST(buf.length() == 11);
        STD_INSIST(memcmp(buf.data(), data, 11) == 0);
    }

    STD_TEST(SequentialReadLines) {
        const char* data = "1\n2\n3\n4\n5\n";
        MemoryInput input(data, 10);
        for (u8 i = 1; i <= 5; ++i) {
            Buffer buf;
            input.readLine(buf);
            STD_INSIST(buf.length() == 1);
            STD_INSIST(*(u8*)buf.data() == '0' + i);
        }
    }

    STD_TEST(EmptyLinesOnly) {
        const char* data = "\n\n\n\n\n";
        MemoryInput input(data, 5);
        for (size_t i = 0; i < 5; ++i) {
            Buffer buf;
            input.readLine(buf);
            STD_INSIST(buf.empty());
        }
    }

    STD_TEST(MixedContentLines) {
        const char* data = "abc123\n!@#$%^\n\t  \n";
        MemoryInput input(data, 19);
        Buffer buf1, buf2, buf3;
        input.readLine(buf1);
        input.readLine(buf2);
        input.readLine(buf3);
        STD_INSIST(buf1.length() == 6);
        STD_INSIST(memcmp(buf1.data(), "abc123", 6) == 0);
        STD_INSIST(buf2.length() == 6);
        STD_INSIST(memcmp(buf2.data(), "!@#$%^", 6) == 0);
        STD_INSIST(buf3.length() == 3);
        STD_INSIST(memcmp(buf3.data(), "\t  ", 3) == 0);
    }
}
