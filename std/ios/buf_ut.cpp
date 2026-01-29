#include "buf.h"
#include "string.h"
#include "mem.h"

#include <std/tst/ut.h>
#include <std/str/dynamic.h>

using namespace Std;

STD_TEST_SUITE(OutBuf) {
    STD_TEST(basic_write) {
        DynString str;
        {
            StringOutput strOut(str);
            OutBuf buf(strOut);
            buf.write(u8"hello", 5);
        }
        STD_INSIST(StringView(str) == StringView(u8"hello"));
    }

    STD_TEST(multiple_writes) {
        DynString str;
        {
            StringOutput strOut(str);
            OutBuf buf(strOut);
            buf.write(u8"hello", 5);
            buf.write(u8" ", 1);
            buf.write(u8"world", 5);
        }
        STD_INSIST(StringView(str) == StringView(u8"hello world"));
    }

    STD_TEST(flush) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        buf.write(u8"test", 4);
        STD_INSIST(str.empty());

        buf.flush();
        STD_INSIST(StringView(str) == StringView(u8"test"));
    }

    STD_TEST(auto_flush_on_destroy) {
        DynString str;
        StringOutput strOut(str);
        {
            OutBuf buf(strOut);
            buf.write(u8"data", 4);
            STD_INSIST(str.empty());
        }
        STD_INSIST(StringView(str) == StringView(u8"data"));
    }

    STD_TEST(large_write_over_threshold) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        u8 data[20000];
        for (size_t i = 0; i < 20000; ++i) {
            data[i] = (u8)(i % 256);
        }

        buf.write(data, 20000);
        buf.finish();

        STD_INSIST(str.length() == 20000);
    }

    STD_TEST(write_mixed_sizes) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        buf.write(u8"a", 1);
        buf.write(u8"bc", 2);
        buf.write(u8"def", 3);
        buf.write(u8"ghij", 4);
        buf.finish();

        STD_INSIST(StringView(str) == StringView(u8"abcdefghij"));
    }

    STD_TEST(zero_copy_imbue_bump) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        auto ubuf = buf.imbue(10);
        u8* ptr = (u8*)ubuf.ptr;
        ptr[0] = 'x';
        ptr[1] = 'y';
        ptr[2] = 'z';
        buf.bump(ptr + 3);

        buf.finish();
        STD_INSIST(StringView(str) == StringView(u8"xyz"));
    }

    STD_TEST(move_constructor) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf1(strOut);

        buf1.write(u8"test", 4);

        OutBuf buf2(move(buf1));
        buf2.write(u8"123", 3);
        buf2.finish();

        STD_INSIST(StringView(str) == StringView(u8"test123"));
    }

    STD_TEST(xchg) {
        DynString str1, str2;
        StringOutput strOut1(str1);
        StringOutput strOut2(str2);

        OutBuf buf1(strOut1);
        OutBuf buf2(strOut2);

        buf1.write(u8"buf1", 4);
        buf2.write(u8"buf2", 4);

        buf1.xchg(buf2);

        buf2.finish();
        buf1.finish();

        STD_INSIST(StringView(str1) == StringView(u8"buf1"));
        STD_INSIST(StringView(str2) == StringView(u8"buf2"));
    }

    STD_TEST(stream_access) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        STD_INSIST(&buf.stream() == &strOut);
    }

    STD_TEST(finish_then_destroy) {
        DynString str;
        StringOutput strOut(str);
        {
            OutBuf buf(strOut);
            buf.write(u8"data", 4);
            buf.finish();
        }
        STD_INSIST(StringView(str) == StringView(u8"data"));
    }

    STD_TEST(multiple_flushes) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        buf.write(u8"one", 3);
        buf.flush();
        STD_INSIST(StringView(str) == StringView(u8"one"));

        buf.write(u8"two", 3);
        buf.flush();
        STD_INSIST(StringView(str) == StringView(u8"onetwo"));

        buf.write(u8"three", 5);
        buf.finish();
        STD_INSIST(StringView(str) == StringView(u8"onetwothree"));
    }

    STD_TEST(operator_output) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        buf << StringView(u8"test") << 123;
        buf.finish();

        STD_INSIST(str.length() > 0);
    }

    STD_TEST(accumulate_then_flush) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        for (int i = 0; i < 100; ++i) {
            buf.write(u8"x", 1);
        }

        buf.flush();
        STD_INSIST(str.length() == 100);
    }

    STD_TEST(batch_write_near_threshold) {
        DynString str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        u8 data[15000];
        for (size_t i = 0; i < 15000; ++i) {
            data[i] = 'a';
        }

        buf.write(data, 15000);
        buf.write(data, 15000);
        buf.finish();

        STD_INSIST(str.length() == 30000);
    }
}
