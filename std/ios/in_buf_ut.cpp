#include "in_buf.h"
#include "in_mem.h"

#include <std/tst/ut.h>
#include <std/lib/buffer.h>
#include <std/alg/defer.h>

#include <cstring>

using namespace stl;

STD_TEST_SUITE(InBufBasicRead) {
    STD_TEST(EmptyRead) {
        const char* data = "";
        MemoryInput slave(data, 0);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 0);
    }

    STD_TEST(SmallRead) {
        const char* data = "hello";
        MemoryInput slave(data, 5);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 5);
        STD_INSIST(memcmp(chunk, "hello", 5) == 0);
    }

    STD_TEST(MultipleSmallReads) {
        const char* data = "hello world";
        MemoryInput slave(data, 11);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 11);
        STD_INSIST(memcmp(chunk, "hello world", 11) == 0);
        buf.commit(6);
        len = buf.next(&chunk);
        STD_INSIST(len == 5);
        STD_INSIST(memcmp(chunk, "world", 5) == 0);
    }

    STD_TEST(LargeRead) {
        const size_t size = 100000;
        u8* data = new u8[size];
        STD_DEFER {
            delete[] data;
        };
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput slave(data, size);
        InBuf buf(slave);
        Buffer result;
        const void* chunk;
        while (auto len = buf.next(&chunk)) {
            result.append(chunk, len);
            buf.commit(len);
        }
        STD_INSIST(result.length() == size);
        for (size_t i = 0; i < size; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)(i % 256));
        }
    }

    STD_TEST(ReadWithCommit) {
        const char* data = "0123456789";
        MemoryInput slave(data, 10);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 10);
        STD_INSIST(memcmp(chunk, "0123456789", 10) == 0);
        buf.commit(5);
        len = buf.next(&chunk);
        STD_INSIST(len == 5);
        STD_INSIST(memcmp(chunk, "56789", 5) == 0);
        buf.commit(5);
        len = buf.next(&chunk);
        STD_INSIST(len == 0);
    }
}

STD_TEST_SUITE(InBufZeroCopy) {
    STD_TEST(NextCommitSmall) {
        const char* data = "test";
        MemoryInput slave(data, 4);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 4);
        STD_INSIST(memcmp(chunk, "test", 4) == 0);
        buf.commit(4);
    }

    STD_TEST(MultipleNextCommit) {
        const char* data = "hello world";
        MemoryInput slave(data, 11);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 11);
        buf.commit(6);
        len = buf.next(&chunk);
        STD_INSIST(len == 5);
        STD_INSIST(memcmp(chunk, "world", 5) == 0);
        buf.commit(5);
    }

    STD_TEST(PartialCommit) {
        const char* data = "0123456789";
        MemoryInput slave(data, 10);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len >= 10);
        buf.commit(3);
        len = buf.next(&chunk);
        STD_INSIST(len >= 7);
        STD_INSIST(memcmp(chunk, "3456789", 7) == 0);
    }
}

STD_TEST_SUITE(InBufChunkSize) {
    STD_TEST(CustomChunkSize) {
        const size_t totalSize = 2000;
        u8* data = new u8[totalSize];
        STD_DEFER {
            delete[] data;
        };
        for (size_t i = 0; i < totalSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput slave(data, totalSize);
        InBuf buf(slave, 512);
        Buffer result;
        const void* chunk;
        while (auto len = buf.next(&chunk)) {
            result.append(chunk, len);
            buf.commit(len);
        }
        STD_INSIST(result.length() == totalSize);
        for (size_t i = 0; i < totalSize; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)(i % 256));
        }
    }

    STD_TEST(SmallChunkSize) {
        const char* data = "0123456789abcdef0123456789abcdef";
        MemoryInput slave(data, 32);
        InBuf buf(slave, 16);
        const void* chunk1;
        size_t len1 = buf.next(&chunk1);
        STD_INSIST(len1 >= 16);
        STD_INSIST(memcmp(chunk1, "0123456789abcdef", 16) == 0);
        buf.commit(16);
        const void* chunk2;
        size_t len2 = buf.next(&chunk2);
        STD_INSIST(len2 == 16);
        STD_INSIST(memcmp(chunk2, "0123456789abcdef", 16) == 0);
    }

    STD_TEST(LargeChunkSize) {
        const size_t testSize = 10000;
        u8* data = new u8[testSize];
        STD_DEFER {
            delete[] data;
        };
        memset(data, 'A', testSize);
        MemoryInput slave(data, testSize);
        InBuf buf(slave, 65536);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == testSize);
        buf.commit(testSize);
    }
}

STD_TEST_SUITE(InBufReadLine) {
    STD_TEST(SingleLineWithNewline) {
        const char* data = "hello\n";
        MemoryInput slave(data, 6);
        InBuf buf(slave);
        Buffer line;
        bool hasData = buf.readLine(line);
        STD_INSIST(line.length() == 5);
        STD_INSIST(memcmp(line.data(), "hello", 5) == 0);
        STD_INSIST(hasData);
    }

    STD_TEST(MultipleLines) {
        const char* data = "first\nsecond\nthird";
        MemoryInput slave(data, 18);
        InBuf buf(slave);
        Buffer buf1, buf2, buf3;
        bool hasData1 = buf.readLine(buf1);
        bool hasData2 = buf.readLine(buf2);
        bool hasData3 = buf.readLine(buf3);
        STD_INSIST(buf1.length() == 5);
        STD_INSIST(memcmp(buf1.data(), "first", 5) == 0);
        STD_INSIST(hasData1);
        STD_INSIST(buf2.length() == 6);
        STD_INSIST(memcmp(buf2.data(), "second", 6) == 0);
        STD_INSIST(hasData2);
        STD_INSIST(buf3.length() == 5);
        STD_INSIST(memcmp(buf3.data(), "third", 5) == 0);
        STD_INSIST(hasData3);
    }

    STD_TEST(EmptyLine) {
        const char* data = "\n";
        MemoryInput slave(data, 1);
        InBuf buf(slave);
        Buffer line;
        bool hasData = buf.readLine(line);
        STD_INSIST(line.empty());
        STD_INSIST(hasData);
    }
}

STD_TEST_SUITE(InBufMoveSemantics) {
    STD_TEST(MoveConstructor) {
        const char* data = "testdata";
        MemoryInput slave(data, 8);
        InBuf buf1(slave);
        const void* chunk1;
        size_t len1 = buf1.next(&chunk1);
        STD_INSIST(len1 == 8);
        buf1.commit(4);
        InBuf buf2(static_cast<InBuf&&>(buf1));
        const void* chunk2;
        size_t len2 = buf2.next(&chunk2);
        STD_INSIST(len2 == 4);
        STD_INSIST(memcmp(chunk2, "data", 4) == 0);
    }

    STD_TEST(Xchg) {
        const char* data1 = "one";
        const char* data2 = "two";
        MemoryInput slave1(data1, 3);
        MemoryInput slave2(data2, 3);
        InBuf buf1(slave1);
        InBuf buf2(slave2);
        const void* chunk;
        buf1.next(&chunk);
        buf1.commit(1);
        buf2.next(&chunk);
        buf2.commit(1);
        buf1.xchg(buf2);
        size_t len = buf1.next(&chunk);
        STD_INSIST(len == 2);
        STD_INSIST(memcmp(chunk, "wo", 2) == 0);
        len = buf2.next(&chunk);
        STD_INSIST(len == 2);
        STD_INSIST(memcmp(chunk, "ne", 2) == 0);
    }
}

STD_TEST_SUITE(InBufBinaryData) {
    STD_TEST(NullBytes) {
        u8 data[10] = {0};
        MemoryInput slave(data, 10);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 10);
        for (size_t i = 0; i < 10; ++i) {
            STD_INSIST(((u8*)chunk)[i] == 0);
        }
    }

    STD_TEST(MaxByteValues) {
        u8 data[10];
        for (size_t i = 0; i < 10; ++i) {
            data[i] = 0xFF;
        }
        MemoryInput slave(data, 10);
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == 10);
        for (size_t i = 0; i < 10; ++i) {
            STD_INSIST(((u8*)chunk)[i] == 0xFF);
        }
    }

    STD_TEST(MixedBinaryData) {
        u8 data[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
        MemoryInput slave(data, sizeof(data));
        InBuf buf(slave);
        const void* chunk;
        size_t len = buf.next(&chunk);
        STD_INSIST(len == sizeof(data));
        STD_INSIST(memcmp(chunk, data, sizeof(data)) == 0);
    }

    STD_TEST(BinaryPattern) {
        const size_t size = 1000;
        u8* data = new u8[size];
        STD_DEFER {
            delete[] data;
        };
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput slave(data, size);
        InBuf buf(slave);
        Buffer result;
        const void* chunk;
        while (auto len = buf.next(&chunk)) {
            result.append(chunk, len);
            buf.commit(len);
        }
        STD_INSIST(result.length() == size);
        for (size_t i = 0; i < size; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)(i % 256));
        }
    }
}

STD_TEST_SUITE(InBufReadMethod) {
    STD_TEST(ReadDirect) {
        const char* data = "hello world";
        MemoryInput slave(data, 11);
        InBuf buf(slave);
        u8 buffer[11];
        size_t len = buf.read(buffer, 11);
        STD_INSIST(len == 11);
        STD_INSIST(memcmp(buffer, "hello world", 11) == 0);
    }

    STD_TEST(ReadPartial) {
        const char* data = "hello world";
        MemoryInput slave(data, 11);
        InBuf buf(slave);
        u8 buffer[5];
        size_t len = buf.read(buffer, 5);
        STD_INSIST(len == 5);
        STD_INSIST(memcmp(buffer, "hello", 5) == 0);
        len = buf.read(buffer, 5);
        STD_INSIST(len == 5);
        STD_INSIST(memcmp(buffer, " worl", 5) == 0);
    }

    STD_TEST(ReadAll) {
        const char* data = "test data";
        MemoryInput slave(data, 9);
        InBuf buf(slave);
        Buffer result;
        buf.readAll(result);
        STD_INSIST(result.length() == 9);
        STD_INSIST(memcmp(result.data(), "test data", 9) == 0);
    }
}

STD_TEST_SUITE(InBufHint) {
    STD_TEST(DefaultHint) {
        const char* data = "test";
        MemoryInput slave(data, 4);
        InBuf buf(slave);
        size_t hint = buf.hint();
        STD_INSIST(hint >= (1 << 14));
    }

    STD_TEST(CustomHint) {
        const char* data = "test";
        MemoryInput slave(data, 4);
        InBuf buf(slave, 1024);
        size_t hint = buf.hint();
        STD_INSIST(hint >= 1024);
    }
}
