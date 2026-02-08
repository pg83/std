#include "input.h"
#include "in_mem.h"

#include <std/tst/ut.h>
#include <std/lib/buffer.h>

#include <cstring>

using namespace Std;

STD_TEST_SUITE(InputReadAll) {
    STD_TEST(EmptyInput) {
        const char* data = "";
        MemoryInput input(data, 0);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == 0);
        STD_INSIST(result.empty());
    }

    STD_TEST(SmallInput) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == 5);
        STD_INSIST(memcmp(result.data(), "hello", 5) == 0);
    }

    STD_TEST(MediumInput) {
        const char* data = "Hello, World! This is a test message.";
        const size_t len = strlen(data);
        MemoryInput input(data, len);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == len);
        STD_INSIST(memcmp(result.data(), data, len) == 0);
    }

    STD_TEST(LargeInput) {
        const size_t bufSize = 100000;
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, bufSize);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)(i % 256));
        }
        delete[] data;
    }

    STD_TEST(BinaryData) {
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(data, sizeof(data));
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == sizeof(data));
        STD_INSIST(memcmp(result.data(), data, sizeof(data)) == 0);
    }

    STD_TEST(PrefilledBufferReplaced) {
        const char* data = "new data";
        MemoryInput input(data, 8);
        Buffer result;
        result.append("old", 3);
        STD_INSIST(result.length() == 3);
        input.readAll(result);
        STD_INSIST(result.length() == 11);
        STD_INSIST(memcmp(result.data(), "oldnew data", 8) == 0);
    }

    STD_TEST(MultipleReads) {
        const char* data1 = "first";
        const char* data2 = "second";
        MemoryInput input1(data1, 5);
        MemoryInput input2(data2, 6);
        Buffer result1, result2;
        input1.readAll(result1);
        input2.readAll(result2);
        STD_INSIST(result1.length() == 5);
        STD_INSIST(result2.length() == 6);
        STD_INSIST(memcmp(result1.data(), "first", 5) == 0);
        STD_INSIST(memcmp(result2.data(), "second", 6) == 0);
    }

    STD_TEST(ChunkBoundary) {
        const size_t size = 128;
        u8 data[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)i;
        }
        MemoryInput input(data, size);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == size);
        for (size_t i = 0; i < size; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)i);
        }
    }

    STD_TEST(LargeChunkBoundary) {
        const size_t size = 65536;
        u8* data = new u8[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, size);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == size);
        for (size_t i = 0; i < size; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)(i % 256));
        }
        delete[] data;
    }

    STD_TEST(SingleByte) {
        u8 data = 0xAB;
        MemoryInput input(&data, 1);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == 1);
        STD_INSIST(*(u8*)result.data() == 0xAB);
    }

    STD_TEST(RepeatedPattern) {
        const size_t patternLen = 10;
        const size_t repeats = 1000;
        const size_t totalSize = patternLen * repeats;
        u8* data = new u8[totalSize];
        for (size_t i = 0; i < totalSize; ++i) {
            data[i] = (u8)(i % patternLen);
        }
        MemoryInput input(data, totalSize);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == totalSize);
        for (size_t i = 0; i < totalSize; ++i) {
            STD_INSIST(((u8*)result.data())[i] == (u8)(i % patternLen));
        }
        delete[] data;
    }

    STD_TEST(NullBytes) {
        u8 data[100] = {0};
        MemoryInput input(data, 100);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(((u8*)result.data())[i] == 0);
        }
    }

    STD_TEST(MaxBytes) {
        u8 data[100];
        for (size_t i = 0; i < 100; ++i) {
            data[i] = 0xFF;
        }
        MemoryInput input(data, 100);
        Buffer result;
        input.readAll(result);
        STD_INSIST(result.length() == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(((u8*)result.data())[i] == 0xFF);
        }
    }
}
