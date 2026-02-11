#include "out_buf.h"
#include "out_mem.h"

#include <std/tst/ut.h>
#include <std/str/builder.h>
#include <std/str/view.h>
#include <std/lib/buffer.h>

#include <cstring>

using namespace Std;

STD_TEST_SUITE(OutBufBasicWrite) {
    STD_TEST(EmptyWrite) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.finish();
        STD_INSIST(slave.length() == 0);
    }

    STD_TEST(SmallWrite) {
        StringBuilder slave;
        OutBuf buf(slave);
        const char* data = "hello";
        buf.write(data, 5);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"hello"));
    }

    STD_TEST(MultipleSmallWrites) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("hello", 5);
        buf.write(" ", 1);
        buf.write("world", 5);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"hello world"));
    }

    STD_TEST(LargeWrite) {
        StringBuilder slave;
        OutBuf buf(slave);
        const size_t size = 100000;
        u8* data = new u8[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)(i % 256);
        }
        buf.write(data, size);
        buf.finish();
        STD_INSIST(slave.length() == size);
        StringView sv(slave);
        for (size_t i = 0; i < size; ++i) {
            STD_INSIST(sv.data()[i] == (u8)(i % 256));
        }
        delete[] data;
    }

    STD_TEST(MixedSizeWrites) {
        StringBuilder slave;
        OutBuf buf(slave, 1024);
        buf.write("small", 5);
        const size_t largeSize = 5000;
        u8* largeData = new u8[largeSize];
        memset(largeData, 'X', largeSize);
        buf.write(largeData, largeSize);
        buf.write("end", 3);
        buf.finish();
        STD_INSIST(slave.length() == 5 + largeSize + 3);
        StringView sv(slave);
        STD_INSIST(sv.prefix(5) == StringView(u8"small"));
        STD_INSIST(sv.suffix(3) == StringView(u8"end"));
        delete[] largeData;
    }
}

STD_TEST_SUITE(OutBufZeroCopy) {
    STD_TEST(ImbueCommitSmall) {
        StringBuilder slave;
        OutBuf buf(slave);
        size_t avail = 10;
        void* ptr = buf.imbue(&avail);
        STD_INSIST(ptr != nullptr);
        STD_INSIST(avail >= 10);
        memcpy(ptr, "test", 4);
        buf.commit(4);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"test"));
    }

    STD_TEST(MultipleImbueCommit) {
        StringBuilder slave;
        OutBuf buf(slave);
        size_t avail1 = 5;
        void* ptr1 = buf.imbue(&avail1);
        memcpy(ptr1, "hello", 5);
        buf.commit(5);
        size_t avail2 = 6;
        void* ptr2 = buf.imbue(&avail2);
        memcpy(ptr2, " world", 6);
        buf.commit(6);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"hello world"));
    }

    STD_TEST(LargeImbue) {
        StringBuilder slave;
        OutBuf buf(slave);
        size_t avail = 100000;
        void* ptr = buf.imbue(&avail);
        STD_INSIST(ptr != nullptr);
        for (size_t i = 0; i < 1000; ++i) {
            ((u8*)ptr)[i] = (u8)(i % 256);
        }
        buf.commit(1000);
        buf.finish();
        STD_INSIST(slave.length() == 1000);
        StringView sv(slave);
        for (size_t i = 0; i < 1000; ++i) {
            STD_INSIST(sv.data()[i] == (u8)(i % 256));
        }
    }

    STD_TEST(ImbueWithMixedWrite) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("prefix:", 7);
        size_t avail = 10;
        void* ptr = buf.imbue(&avail);
        memcpy(ptr, "middle", 6);
        buf.commit(6);
        buf.write(":suffix", 7);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"prefix:middle:suffix"));
    }

    STD_TEST(PartialCommit) {
        StringBuilder slave;
        OutBuf buf(slave);
        size_t avail = 100;
        void* ptr = buf.imbue(&avail);
        STD_INSIST(avail >= 100);
        memcpy(ptr, "partial", 7);
        buf.commit(7);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"partial"));
    }
}

STD_TEST_SUITE(OutBufChunkSize) {
    STD_TEST(CustomChunkSize) {
        StringBuilder slave;
        OutBuf buf(slave, 512);
        for (int i = 0; i < 100; ++i) {
            buf.write("test", 4);
        }
        buf.finish();
        STD_INSIST(slave.length() == 400);
    }

    STD_TEST(SmallChunkSize) {
        StringBuilder slave;
        OutBuf buf(slave, 16);
        buf.write("0123456789abcdef", 16);
        buf.write("0123456789abcdef", 16);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"0123456789abcdef0123456789abcdef"));
    }

    STD_TEST(LargeChunkSize) {
        StringBuilder slave;
        OutBuf buf(slave, 65536);
        const size_t testSize = 10000;
        u8* data = new u8[testSize];
        memset(data, 'A', testSize);
        buf.write(data, testSize);
        buf.finish();
        STD_INSIST(slave.length() == testSize);
        delete[] data;
    }

    STD_TEST(WriteExactlyChunkSize) {
        StringBuilder slave;
        OutBuf buf(slave, 1024);
        u8 data[1024];
        memset(data, 'B', 1024);
        buf.write(data, 1024);
        buf.finish();
        STD_INSIST(slave.length() == 1024);
        StringView sv(slave);
        STD_INSIST(sv.data()[0] == 'B');
        STD_INSIST(sv.data()[1023] == 'B');
    }
}

STD_TEST_SUITE(OutBufFlush) {
    STD_TEST(FlushEmpty) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.flush();
        STD_INSIST(slave.length() == 0);
    }

    STD_TEST(FlushAfterWrite) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("data", 4);
        buf.flush();
        STD_INSIST(StringView(slave) == StringView(u8"data"));
    }

    STD_TEST(FlushMultipleTimes) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("first", 5);
        buf.flush();
        STD_INSIST(slave.length() == 5);
        buf.write("second", 6);
        buf.flush();
        STD_INSIST(StringView(slave) == StringView(u8"firstsecond"));
    }

    STD_TEST(FlushAfterImbue) {
        StringBuilder slave;
        OutBuf buf(slave);
        size_t avail = 10;
        void* ptr = buf.imbue(&avail);
        memcpy(ptr, "imbued", 6);
        buf.commit(6);
        buf.flush();
        STD_INSIST(StringView(slave) == StringView(u8"imbued"));
    }

    STD_TEST(FlushBeforeFinish) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("test", 4);
        buf.flush();
        buf.write("data", 4);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"testdata"));
    }
}

STD_TEST_SUITE(OutBufMoveSemantics) {
    STD_TEST(MoveConstructor) {
        StringBuilder slave;
        OutBuf buf1(slave);
        buf1.write("test", 4);
        OutBuf buf2(static_cast<OutBuf&&>(buf1));
        buf2.write("data", 4);
        buf2.finish();
        STD_INSIST(StringView(slave) == StringView(u8"testdata"));
    }

    STD_TEST(MoveAfterWrites) {
        StringBuilder slave;
        OutBuf buf1(slave);
        buf1.write("hello", 5);
        buf1.write(" ", 1);
        OutBuf buf2(static_cast<OutBuf&&>(buf1));
        buf2.write("world", 5);
        buf2.finish();
        STD_INSIST(StringView(slave) == StringView(u8"hello world"));
    }

    STD_TEST(Xchg) {
        StringBuilder slave1;
        StringBuilder slave2;
        OutBuf buf1(slave1);
        OutBuf buf2(slave2);
        buf1.write("one", 3);
        buf2.write("two", 3);
        buf1.xchg(buf2);
        buf1.finish();
        buf2.finish();
        STD_INSIST(StringView(slave2) == StringView(u8"two"));
        STD_INSIST(StringView(slave1) == StringView(u8"one"));
    }
}

STD_TEST_SUITE(OutBufStreamOperator) {
    STD_TEST(StreamInteger) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf << 42;
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"42"));
    }

    STD_TEST(StreamString) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf << StringView(u8"hello");
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"hello"));
    }

    STD_TEST(StreamMultiple) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf << 1 << StringView(u8" ") << 2 << StringView(u8" ") << 3;
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"1 2 3"));
    }

    STD_TEST(StreamMixed) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf << StringView(u8"The answer is ") << 42;
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"The answer is 42"));
    }

    STD_TEST(StreamNegativeNumbers) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf << -100 << StringView(u8" ") << -1;
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"-100 -1"));
    }
}

STD_TEST_SUITE(OutBufEdgeCases) {
    STD_TEST(ZeroLengthWrite) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("test", 0);
        buf.finish();
        STD_INSIST(slave.length() == 0);
    }

    STD_TEST(AlternatingWriteImbue) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("A", 1);
        size_t avail = 10;
        void* ptr = buf.imbue(&avail);
        memcpy(ptr, "B", 1);
        buf.commit(1);
        buf.write("C", 1);
        buf.imbue(&avail);
        memcpy(ptr, "D", 1);
        buf.commit(1);
        buf.finish();
        STD_INSIST(slave.length() == 4);
    }

    STD_TEST(RepeatedFlush) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("test", 4);
        buf.flush();
        buf.flush();
        buf.flush();
        STD_INSIST(slave.length() == 4);
        STD_INSIST(memcmp(slave.data(), "test", 4) == 0);
    }

    STD_TEST(WriteAfterFlush) {
        StringBuilder slave;
        OutBuf buf(slave);
        buf.write("first", 5);
        buf.flush();
        size_t firstLen = slave.length();
        STD_INSIST(firstLen == 5);
        buf.write("second", 6);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"firstsecond"));
    }

    STD_TEST(LargeNumberOfSmallWrites) {
        StringBuilder slave;
        OutBuf buf(slave, 256);
        for (int i = 0; i < 1000; ++i) {
            buf.write("x", 1);
        }
        buf.finish();
        STD_INSIST(slave.length() == 1000);
        StringView sv(slave);
        for (size_t i = 0; i < 1000; ++i) {
            STD_INSIST(sv.data()[i] == 'x');
        }
    }
}

STD_TEST_SUITE(OutBufBinaryData) {
    STD_TEST(NullBytes) {
        StringBuilder slave;
        OutBuf buf(slave);
        u8 data[10] = {0};
        buf.write(data, 10);
        buf.finish();
        STD_INSIST(slave.length() == 10);
        StringView sv(slave);
        for (size_t i = 0; i < 10; ++i) {
            STD_INSIST(sv.data()[i] == 0);
        }
    }

    STD_TEST(MaxByteValues) {
        StringBuilder slave;
        OutBuf buf(slave);
        u8 data[10];
        for (size_t i = 0; i < 10; ++i) {
            data[i] = 0xFF;
        }
        buf.write(data, 10);
        buf.finish();
        STD_INSIST(slave.length() == 10);
        StringView sv(slave);
        for (size_t i = 0; i < 10; ++i) {
            STD_INSIST(sv.data()[i] == 0xFF);
        }
    }

    STD_TEST(MixedBinaryData) {
        StringBuilder slave;
        OutBuf buf(slave);
        u8 data[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
        buf.write(data, sizeof(data));
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(data, sizeof(data)));
    }

    STD_TEST(BinaryPattern) {
        StringBuilder slave;
        OutBuf buf(slave);
        const size_t size = 1000;
        u8* data = new u8[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)(i % 256);
        }
        buf.write(data, size);
        buf.finish();
        STD_INSIST(slave.length() == size);
        StringView sv(slave);
        for (size_t i = 0; i < size; ++i) {
            STD_INSIST(sv.data()[i] == (u8)(i % 256));
        }
        delete[] data;
    }
}

STD_TEST_SUITE(OutBufWithMemoryOutput) {
    STD_TEST(WriteToMemory) {
        u8 buffer[100];
        MemoryOutput mem(buffer);
        OutBuf buf(mem);
        buf.write("test", 4);
        buf.finish();
        STD_INSIST(StringView(buffer, 4) == StringView(u8"test"));
    }

    STD_TEST(MultipleWritesToMemory) {
        u8 buffer[100];
        MemoryOutput mem(buffer);
        OutBuf buf(mem);
        buf.write("hello", 5);
        buf.write(" ", 1);
        buf.write("world", 5);
        buf.finish();
        STD_INSIST(StringView(buffer, 11) == StringView(u8"hello world"));
    }

    STD_TEST(ImbueToMemory) {
        u8 buffer[100];
        MemoryOutput mem(buffer);
        OutBuf buf(mem);
        size_t avail = 20;
        void* ptr = buf.imbue(&avail);
        memcpy(ptr, "imbued", 6);
        buf.commit(6);
        buf.finish();
        STD_INSIST(StringView(buffer, 6) == StringView(u8"imbued"));
    }
}

STD_TEST_SUITE(OutBufBufferManagement) {
    STD_TEST(BufferGrowth) {
        StringBuilder slave;
        OutBuf buf(slave, 64);
        for (int i = 0; i < 100; ++i) {
            buf.write("test", 4);
        }
        buf.finish();
        STD_INSIST(slave.length() == 400);
    }

    STD_TEST(BufferReuse) {
        StringBuilder slave;
        OutBuf buf(slave, 128);
        buf.write("data", 4);
        buf.flush();
        buf.write("more", 4);
        buf.flush();
        buf.write("data", 4);
        buf.finish();
        STD_INSIST(StringView(slave) == StringView(u8"datamoredata"));
    }

    STD_TEST(InternalBufferFill) {
        StringBuilder slave;
        OutBuf buf(slave, 1024);
        u8 data[512];
        memset(data, 'A', 512);
        buf.write(data, 512);
        memset(data, 'B', 512);
        buf.write(data, 512);
        buf.finish();
        STD_INSIST(slave.length() == 1024);
    }
}
