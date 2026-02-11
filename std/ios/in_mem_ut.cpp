#include "in_mem.h"
#include "out_mem.h"

#include <std/tst/ut.h>
#include <std/str/builder.h>

#include <cstring>

using namespace Std;

namespace {
    static inline void copy(Input& in, Output& out) {
        in.sendTo(out);
    }
}

STD_TEST_SUITE(MemoryInputAsInput) {
    STD_TEST(EmptyRead) {
        const char* data = "";
        MemoryInput input(data, 0);
        u8 buffer[10];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 0);
    }

    STD_TEST(SmallRead) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        u8 buffer[10];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 5);
        STD_INSIST(memcmp(buffer, "hello", 5) == 0);
    }

    STD_TEST(ExactBufferRead) {
        const char* data = "exact";
        MemoryInput input(data, 5);
        u8 buffer[5];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 5);
        STD_INSIST(memcmp(buffer, "exact", 5) == 0);
    }

    STD_TEST(PartialRead) {
        const char* data = "hello world";
        MemoryInput input(data, 11);
        u8 buffer[5];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 5);
        STD_INSIST(memcmp(buffer, "hello", 5) == 0);
    }

    STD_TEST(MultipleReads) {
        const char* data = "hello world";
        MemoryInput input(data, 11);
        u8 buffer1[5];
        u8 buffer2[6];
        size_t bytesRead1 = input.read(buffer1, sizeof(buffer1));
        size_t bytesRead2 = input.read(buffer2, sizeof(buffer2));
        STD_INSIST(bytesRead1 == 5);
        STD_INSIST(bytesRead2 == 6);
        STD_INSIST(memcmp(buffer1, "hello", 5) == 0);
        STD_INSIST(memcmp(buffer2, " world", 6) == 0);
    }

    STD_TEST(ReadAfterExhausted) {
        const char* data = "test";
        MemoryInput input(data, 4);
        u8 buffer1[4];
        u8 buffer2[4];
        size_t bytesRead1 = input.read(buffer1, sizeof(buffer1));
        size_t bytesRead2 = input.read(buffer2, sizeof(buffer2));
        STD_INSIST(bytesRead1 == 4);
        STD_INSIST(bytesRead2 == 0);
        STD_INSIST(memcmp(buffer1, "test", 4) == 0);
    }

    STD_TEST(BinaryDataRead) {
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(data, sizeof(data));
        u8 buffer[sizeof(data)];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == sizeof(data));
        STD_INSIST(memcmp(buffer, data, sizeof(data)) == 0);
    }

    STD_TEST(LargeRead) {
        const size_t bufSize = 100000;
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, bufSize);
        u8* buffer = new u8[bufSize];
        size_t bytesRead = input.read(buffer, bufSize);
        STD_INSIST(bytesRead == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(buffer[i] == (u8)(i % 256));
        }
        delete[] data;
        delete[] buffer;
    }

    STD_TEST(SingleByteRead) {
        u8 data = 0xAB;
        MemoryInput input(&data, 1);
        u8 buffer;
        size_t bytesRead = input.read(&buffer, 1);
        STD_INSIST(bytesRead == 1);
        STD_INSIST(buffer == 0xAB);
    }

    STD_TEST(IncrementalReads) {
        const char* data = "0123456789";
        MemoryInput input(data, 10);
        u8 buffer;
        for (size_t i = 0; i < 10; ++i) {
            size_t bytesRead = input.read(&buffer, 1);
            STD_INSIST(bytesRead == 1);
            STD_INSIST(buffer == '0' + i);
        }
        size_t bytesRead = input.read(&buffer, 1);
        STD_INSIST(bytesRead == 0);
    }

    STD_TEST(NullBytesRead) {
        u8 data[100] = {0};
        MemoryInput input(data, 100);
        u8 buffer[100];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(buffer[i] == 0);
        }
    }

    STD_TEST(MaxBytesRead) {
        u8 data[50];
        for (size_t i = 0; i < 50; ++i) {
            data[i] = 0xFF;
        }
        MemoryInput input(data, 50);
        u8 buffer[50];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 50);
        for (size_t i = 0; i < 50; ++i) {
            STD_INSIST(buffer[i] == 0xFF);
        }
    }
}

STD_TEST_SUITE(MemoryInputAsZeroCopy) {
    STD_TEST(EmptyNext) {
        const char* data = "";
        MemoryInput input(data, 0);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 0);
    }

    STD_TEST(SmallNext) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 5);
        STD_INSIST(memcmp(chunk, "hello", 5) == 0);
    }

    STD_TEST(NextCommitPartial) {
        const char* data = "hello world";
        MemoryInput input(data, 11);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 11);
        STD_INSIST(memcmp(chunk, "hello world", 11) == 0);
        input.commit(5);
        available = input.next(&chunk);
        STD_INSIST(available == 6);
        STD_INSIST(memcmp(chunk, " world", 6) == 0);
    }

    STD_TEST(NextCommitFull) {
        const char* data = "test";
        MemoryInput input(data, 4);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 4);
        input.commit(4);
        available = input.next(&chunk);
        STD_INSIST(available == 0);
    }

    STD_TEST(MultipleCommits) {
        const char* data = "0123456789";
        MemoryInput input(data, 10);
        const void* chunk;
        for (size_t i = 0; i < 10; ++i) {
            size_t available = input.next(&chunk);
            STD_INSIST(available == 10 - i);
            STD_INSIST(*(const char*)chunk == '0' + i);
            input.commit(1);
        }
        size_t available = input.next(&chunk);
        STD_INSIST(available == 0);
    }

    STD_TEST(CommitZero) {
        const char* data = "test";
        MemoryInput input(data, 4);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 4);
        input.commit(0);
        available = input.next(&chunk);
        STD_INSIST(available == 4);
        STD_INSIST(memcmp(chunk, "test", 4) == 0);
    }

    STD_TEST(LargeNext) {
        const size_t bufSize = 100000;
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, bufSize);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(((const u8*)chunk)[i] == (u8)(i % 256));
        }
        delete[] data;
    }

    STD_TEST(BinaryDataNext) {
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(data, sizeof(data));
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == sizeof(data));
        STD_INSIST(memcmp(chunk, data, sizeof(data)) == 0);
    }

    STD_TEST(SingleByteNext) {
        u8 data = 0xCD;
        MemoryInput input(&data, 1);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 1);
        STD_INSIST(*(const u8*)chunk == 0xCD);
    }

    STD_TEST(MultipleNextCalls) {
        const char* data = "test";
        MemoryInput input(data, 4);
        const void* chunk1;
        const void* chunk2;
        size_t available1 = input.next(&chunk1);
        size_t available2 = input.next(&chunk2);
        STD_INSIST(available1 == 4);
        STD_INSIST(available2 == 4);
        STD_INSIST(chunk1 == chunk2);
        STD_INSIST(memcmp(chunk1, "test", 4) == 0);
    }

    STD_TEST(NextAfterPartialCommit) {
        const char* data = "0123456789";
        MemoryInput input(data, 10);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 10);
        input.commit(3);
        available = input.next(&chunk);
        STD_INSIST(available == 7);
        STD_INSIST(memcmp(chunk, "3456789", 7) == 0);
        input.commit(3);
        available = input.next(&chunk);
        STD_INSIST(available == 4);
        STD_INSIST(memcmp(chunk, "6789", 4) == 0);
    }

    STD_TEST(NullBytesNext) {
        u8 data[100] = {0};
        MemoryInput input(data, 100);
        const void* chunk;
        size_t available = input.next(&chunk);
        STD_INSIST(available == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(((const u8*)chunk)[i] == 0);
        }
    }
}

STD_TEST_SUITE(MemoryInputWithCopy) {
    STD_TEST(CopyEmpty) {
        const char* data = "";
        MemoryInput input(data, 0);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == 0);
    }

    STD_TEST(CopySmall) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == 5);
        STD_INSIST(memcmp(output.data(), "hello", 5) == 0);
    }

    STD_TEST(CopyMedium) {
        const char* data = "Hello, World! This is a test message for copy function.";
        const size_t len = strlen(data);
        MemoryInput input(data, len);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == len);
        STD_INSIST(memcmp(output.data(), data, len) == 0);
    }

    STD_TEST(CopyLarge) {
        const size_t bufSize = 100000;
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, bufSize);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(((u8*)output.data())[i] == (u8)(i % 256));
        }
        delete[] data;
    }

    STD_TEST(CopyBinaryData) {
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(data, sizeof(data));
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == sizeof(data));
        STD_INSIST(memcmp(output.data(), data, sizeof(data)) == 0);
    }

    STD_TEST(CopySingleByte) {
        u8 data = 0xEF;
        MemoryInput input(&data, 1);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == 1);
        STD_INSIST(*(u8*)output.data() == 0xEF);
    }

    STD_TEST(CopyNullBytes) {
        u8 data[100] = {0};
        MemoryInput input(data, 100);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(((u8*)output.data())[i] == 0);
        }
    }

    STD_TEST(CopyMaxBytes) {
        u8 data[50];
        for (size_t i = 0; i < 50; ++i) {
            data[i] = 0xFF;
        }
        MemoryInput input(data, 50);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == 50);
        for (size_t i = 0; i < 50; ++i) {
            STD_INSIST(((u8*)output.data())[i] == 0xFF);
        }
    }

    STD_TEST(CopyAfterPartialRead) {
        const char* data = "hello world";
        MemoryInput input(data, 11);
        u8 buffer[5];
        size_t bytesRead = input.read(buffer, sizeof(buffer));
        STD_INSIST(bytesRead == 5);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == 6);
        STD_INSIST(memcmp(output.data(), " world", 6) == 0);
    }

    STD_TEST(CopyToMemoryOutput) {
        const char* data = "test data";
        MemoryInput input(data, 9);
        u8 buffer[20];
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(memcmp(buffer, "test data", 9) == 0);
    }

    STD_TEST(CopyRepeatedPattern) {
        const size_t patternLen = 10;
        const size_t repeats = 500;
        const size_t totalSize = patternLen * repeats;
        u8* data = new u8[totalSize];
        for (size_t i = 0; i < totalSize; ++i) {
            data[i] = (u8)(i % patternLen);
        }
        MemoryInput input(data, totalSize);
        StringBuilder output;
        copy(input, output);
        STD_INSIST(output.length() == totalSize);
        for (size_t i = 0; i < totalSize; ++i) {
            STD_INSIST(((u8*)output.data())[i] == (u8)(i % patternLen));
        }
        delete[] data;
    }

    STD_TEST(MultipleCopies) {
        const char* data1 = "first";
        const char* data2 = "second";
        MemoryInput input1(data1, 5);
        MemoryInput input2(data2, 6);
        StringBuilder output1, output2;
        copy(input1, output1);
        copy(input2, output2);
        STD_INSIST(output1.length() == 5);
        STD_INSIST(output2.length() == 6);
        STD_INSIST(memcmp(output1.data(), "first", 5) == 0);
        STD_INSIST(memcmp(output2.data(), "second", 6) == 0);
    }
}
