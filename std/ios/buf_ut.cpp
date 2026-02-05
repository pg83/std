#include "buf.h"
#include "mem.h"

#include <std/tst/ut.h>
#include <std/typ/support.h>

using namespace Std;

namespace {
    struct StringOutput: public Output {
        Buffer* buf;

        inline StringOutput(Buffer& b) noexcept
            : buf(&b)
        {
        }

        size_t writeImpl(const void* data, size_t len) override {
            buf->append(data, len);

            return len;
        }
    };

    struct CountingOutput: public Output {
        u64 len_;

        size_t writeImpl(const void* ptr, size_t len) override {
            len_ += len;
            return len;
        }

        inline CountingOutput() noexcept
            : len_(0)
        {
        }

        inline auto collectedLength() const noexcept {
            return len_;
        }
    };

}

STD_TEST_SUITE(OutBuf) {
    STD_TEST(basic_write) {
        Buffer str;
        {
            StringOutput strOut(str);
            OutBuf buf(strOut);
            buf.write(u8"hello", 5);
        }
        STD_INSIST(StringView(str) == StringView(u8"hello"));
    }

    STD_TEST(multiple_writes) {
        Buffer str;
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
        Buffer str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        buf.write(u8"test", 4);
        STD_INSIST(str.empty());

        buf.flush();
        STD_INSIST(StringView(str) == StringView(u8"test"));
    }

    STD_TEST(auto_flush_on_destroy) {
        Buffer str;
        StringOutput strOut(str);
        {
            OutBuf buf(strOut);
            buf.write(u8"data", 4);
            STD_INSIST(str.empty());
        }
        STD_INSIST(StringView(str) == StringView(u8"data"));
    }

    STD_TEST(large_write_over_threshold) {
        Buffer str;
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
        Buffer str;
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
        Buffer str;
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
        Buffer str;
        StringOutput strOut(str);
        OutBuf buf1(strOut);

        buf1.write(u8"test", 4);

        OutBuf buf2(move(buf1));
        buf2.write(u8"123", 3);
        buf2.finish();

        STD_INSIST(StringView(str) == StringView(u8"test123"));
    }

    STD_TEST(xchg) {
        Buffer str1, str2;
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
        Buffer str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        STD_INSIST(&buf.stream() == &strOut);
    }

    STD_TEST(finish_then_destroy) {
        Buffer str;
        StringOutput strOut(str);
        {
            OutBuf buf(strOut);
            buf.write(u8"data", 4);
            buf.finish();
        }
        STD_INSIST(StringView(str) == StringView(u8"data"));
    }

    STD_TEST(multiple_flushes) {
        Buffer str;
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
        Buffer str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        buf << StringView(u8"test") << 123;
        buf.finish();

        STD_INSIST(str.length() > 0);
    }

    STD_TEST(accumulate_then_flush) {
        Buffer str;
        StringOutput strOut(str);
        OutBuf buf(strOut);

        for (int i = 0; i < 100; ++i) {
            buf.write(u8"x", 1);
        }

        buf.flush();
        STD_INSIST(str.length() == 100);
    }

    STD_TEST(batch_write_near_threshold) {
        Buffer str;
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

    STD_TEST(chunkSizeExactMultiple) {
        CountingOutput counter;
        OutBuf buf(counter, 1024);

        u8 data[1024];
        buf.write(data, 1024);

        STD_INSIST(counter.collectedLength() == 1024);
    }

    STD_TEST(chunkSizeTwoExactMultiples) {
        CountingOutput counter;
        OutBuf buf(counter, 1024);

        u8 data[2048];
        buf.write(data, 2048);

        STD_INSIST(counter.collectedLength() == 2048);
    }

    STD_TEST(chunkSizeNotMultiple) {
        CountingOutput counter;
        OutBuf buf(counter, 1024);

        u8 data[1500];
        buf.write(data, 1500);

        STD_INSIST(counter.collectedLength() == 1024);

        buf.flush();
        STD_INSIST(counter.collectedLength() == 1500);
    }

    STD_TEST(chunkSizeSmallWrites) {
        CountingOutput counter;
        OutBuf buf(counter, 1024);

        u8 data[100];
        for (int i = 0; i < 10; ++i) {
            buf.write(data, 100);
        }

        STD_INSIST(counter.collectedLength() == 0);

        buf.write(data, 100);

        STD_INSIST(counter.collectedLength() == 1024);
    }

    STD_TEST(chunkSizeMixedWrites) {
        CountingOutput counter;
        OutBuf buf(counter, 512);

        u8 data[200];
        buf.write(data, 200);
        STD_INSIST(counter.collectedLength() == 0);

        buf.write(data, 200);
        STD_INSIST(counter.collectedLength() == 0);

        buf.write(data, 200);
        STD_INSIST(counter.collectedLength() == 512);

        buf.write(data, 200);
        STD_INSIST(counter.collectedLength() == 512);

        buf.flush();
        STD_INSIST(counter.collectedLength() == 800);
    }

    STD_TEST(chunkSizeLargeWrite) {
        CountingOutput counter;
        OutBuf buf(counter, 256);

        u8 data[1000];
        buf.write(data, 1000);

        STD_INSIST(counter.collectedLength() == 768);

        buf.flush();
        STD_INSIST(counter.collectedLength() == 1000);
    }

    STD_TEST(chunkSizeBufferPlusLarge) {
        CountingOutput counter;
        OutBuf buf(counter, 512);

        u8 data1[100];
        u8 data2[1000];

        buf.write(data1, 100);
        STD_INSIST(counter.collectedLength() == 0);

        buf.write(data2, 1000);
        STD_INSIST(counter.collectedLength() == 1024);

        buf.flush();
        STD_INSIST(counter.collectedLength() == 1100);
    }

    STD_TEST(chunkSizeZeroCopy) {
        CountingOutput counter;
        OutBuf buf(counter, 512);

        auto ubuf = buf.imbue(1024);
        u8* ptr = (u8*)ubuf.ptr;
        for (size_t i = 0; i < 1024; ++i) {
            ptr[i] = (u8)i;
        }

        STD_INSIST(counter.collectedLength() == 0);

        buf.bump(ptr + 1024);

        STD_INSIST(counter.collectedLength() == 1024);
    }

    STD_TEST(chunkSizeZeroCopyPartial) {
        CountingOutput counter;
        OutBuf buf(counter, 512);

        auto ubuf = buf.imbue(300);
        u8* ptr = (u8*)ubuf.ptr;
        buf.bump(ptr + 300);

        STD_INSIST(counter.collectedLength() == 0);

        ubuf = buf.imbue(300);
        ptr = (u8*)ubuf.ptr;
        buf.bump(ptr + 300);

        STD_INSIST(counter.collectedLength() == 512);
    }

    STD_TEST(chunkSizeEdgeCaseOne) {
        CountingOutput counter;
        OutBuf buf(counter, 1024);

        u8 data[1023];
        buf.write(data, 1023);
        STD_INSIST(counter.collectedLength() == 0);

        u8 one[1];
        buf.write(one, 1);
        STD_INSIST(counter.collectedLength() == 1024);
    }

    STD_TEST(chunkSizeEdgeCasePlusOne) {
        CountingOutput counter;
        OutBuf buf(counter, 1024);

        u8 data[1025];
        buf.write(data, 1025);

        STD_INSIST(counter.collectedLength() == 1024);

        buf.flush();
        STD_INSIST(counter.collectedLength() == 1025);
    }

    STD_TEST(chunkSizeMultipleFlushes) {
        CountingOutput counter;
        OutBuf buf(counter, 256);

        u8 data[100];

        buf.write(data, 100);
        buf.flush();
        STD_INSIST(counter.collectedLength() == 100);

        buf.write(data, 100);
        buf.flush();
        STD_INSIST(counter.collectedLength() == 200);

        buf.write(data, 100);
        STD_INSIST(counter.collectedLength() == 200);
        buf.flush();
        STD_INSIST(counter.collectedLength() == 300);
    }
}
