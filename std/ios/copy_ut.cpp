#include "copy.h"
#include "input.h"
#include "in_mem.h"
#include "output.h"

#include <std/tst/ut.h>
#include <std/str/builder.h>

#include <cstring>

using namespace Std;

namespace {
    static inline void copy(Input& in, Output& out) {
        in.sendTo(out);
    }

    struct CountingOutput: public Output {
        u64 len_;

        size_t writeImpl(const void* ptr, size_t len) override {
            return (len_ += len, len);
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

STD_TEST_SUITE(ZeroCopy) {
    STD_TEST(Empty) {
        const char* data = "";
        MemoryInput input(data, 0);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == 0);
    }

    STD_TEST(Small) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == 5);
        STD_INSIST(memcmp(output.data(), "hello", 5) == 0);
    }

    STD_TEST(Medium) {
        const char* data = "Hello, World! This is a medium test message for copy function.";
        const size_t len = strlen(data);
        MemoryInput input(data, len);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == len);
        STD_INSIST(memcmp(output.data(), data, len) == 0);
    }

    STD_TEST(Large) {
        const size_t bufSize = 100000;
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, bufSize);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(((u8*)output.data())[i] == (u8)(i % 256));
        }
        delete[] data;
    }

    STD_TEST(BinaryData) {
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(data, sizeof(data));
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == sizeof(data));
        STD_INSIST(memcmp(output.data(), data, sizeof(data)) == 0);
    }

    STD_TEST(ChunkBoundary128) {
        const size_t size = 128;
        u8 data[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)i;
        }
        MemoryInput input(data, size);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == size);
        STD_INSIST(memcmp(output.data(), data, size) == 0);
    }

    STD_TEST(ChunkBoundary256) {
        const size_t size = 256;
        u8 data[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)i;
        }
        MemoryInput input(data, size);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == size);
        STD_INSIST(memcmp(output.data(), data, size) == 0);
    }

    STD_TEST(ChunkBoundary65536) {
        const size_t size = 65536;
        u8* data = new u8[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, size);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == size);
        STD_INSIST(memcmp(output.data(), data, size) == 0);
        delete[] data;
    }

    STD_TEST(MultipleCalls) {
        const char* data1 = "first";
        const char* data2 = "second";
        MemoryInput input1(data1, 5);
        MemoryInput input2(data2, 6);
        StringBuilder output1, output2;
        copyIZ(input1, output1);
        copyIZ(input2, output2);
        STD_INSIST(output1.length() == 5);
        STD_INSIST(output2.length() == 6);
        STD_INSIST(memcmp(output1.data(), "first", 5) == 0);
        STD_INSIST(memcmp(output2.data(), "second", 6) == 0);
    }

    STD_TEST(NullBytes) {
        u8 data[100] = {0};
        MemoryInput input(data, 100);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(((u8*)output.data())[i] == 0);
        }
    }

    STD_TEST(SingleByte) {
        u8 data = 0xAB;
        MemoryInput input(&data, 1);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == 1);
        STD_INSIST(*(u8*)output.data() == 0xAB);
    }

    STD_TEST(RepeatedPattern) {
        const size_t patternLen = 10;
        const size_t repeats = 500;
        const size_t totalSize = patternLen * repeats;
        u8* data = new u8[totalSize];
        for (size_t i = 0; i < totalSize; ++i) {
            data[i] = (u8)(i % patternLen);
        }
        MemoryInput input(data, totalSize);
        StringBuilder output;
        copyIZ(input, output);
        STD_INSIST(output.length() == totalSize);
        for (size_t i = 0; i < totalSize; ++i) {
            STD_INSIST(((u8*)output.data())[i] == (u8)(i % patternLen));
        }
        delete[] data;
    }
}

STD_TEST_SUITE(Copy) {
    STD_TEST(Empty) {
        const char* data = "";
        MemoryInput input(data, 0);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == 0);
    }

    STD_TEST(Small) {
        const char* data = "hello";
        MemoryInput input(data, 5);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == 5);
    }

    STD_TEST(Medium) {
        const char* data = "Hello, World! This is a test message for copy.";
        const size_t len = strlen(data);
        MemoryInput input(data, len);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == len);
    }

    STD_TEST(Large) {
        const size_t bufSize = 100000;
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        MemoryInput input(data, bufSize);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == bufSize);
        delete[] data;
    }

    STD_TEST(BinaryData) {
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(data, sizeof(data));
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == sizeof(data));
    }

    STD_TEST(ChunkBoundary) {
        const size_t size = 128;
        u8 data[size];
        for (size_t i = 0; i < size; ++i) {
            data[i] = (u8)i;
        }
        MemoryInput input(data, size);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == size);
    }

    STD_TEST(WithStringBuilder) {
        const char* data = "test data for StringBuilder";
        const size_t len = strlen(data);
        MemoryInput input(data, len);
        StringBuilder sb;

        struct StringBuilderAsOutput: public Output {
            StringBuilder* sb_;

            inline StringBuilderAsOutput(StringBuilder& sb) noexcept
                : sb_(&sb)
            {
            }

            size_t writeImpl(const void* ptr, size_t len) override {
                return (sb_->write(ptr, len), len);
            }
        };

        StringBuilderAsOutput output(sb);
        copy(input, output);
        STD_INSIST(sb.length() == len);
        STD_INSIST(memcmp(sb.data(), data, len) == 0);
    }

    STD_TEST(MultipleCalls) {
        const char* data1 = "first";
        const char* data2 = "second";
        MemoryInput input1(data1, 5);
        MemoryInput input2(data2, 6);
        CountingOutput output1, output2;
        copy(input1, output1);
        copy(input2, output2);
        STD_INSIST(output1.collectedLength() == 5);
        STD_INSIST(output2.collectedLength() == 6);
    }

    STD_TEST(NullBytes) {
        u8 data[100] = {0};
        MemoryInput input(data, 100);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == 100);
    }

    STD_TEST(SingleByte) {
        u8 data = 0xCD;
        MemoryInput input(&data, 1);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == 1);
    }

    STD_TEST(RepeatedPattern) {
        const size_t patternLen = 10;
        const size_t repeats = 500;
        const size_t totalSize = patternLen * repeats;
        u8* data = new u8[totalSize];
        for (size_t i = 0; i < totalSize; ++i) {
            data[i] = (u8)(i % patternLen);
        }
        MemoryInput input(data, totalSize);
        CountingOutput output;
        copy(input, output);
        STD_INSIST(output.collectedLength() == totalSize);
        delete[] data;
    }
}
