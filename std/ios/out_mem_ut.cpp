#include "out_mem.h"
#include "in_mem.h"
#include "in_fd.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>

#include <cstring>

using namespace Std;

namespace {
    static inline void copy(Input& in, Output& out) {
        in.sendTo(out);
    }
}

STD_TEST_SUITE(MemoryOutputAsOutput) {
    STD_TEST(EmptyWrite) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        const char* data = "";
        size_t bytesWritten = output.write(data, 0);
        STD_INSIST(bytesWritten == 0);
    }

    STD_TEST(SmallWrite) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        const char* data = "hello";
        size_t bytesWritten = output.write(data, 5);
        STD_INSIST(bytesWritten == 5);
        STD_INSIST(memcmp(buffer, "hello", 5) == 0);
    }

    STD_TEST(ExactBufferWrite) {
        u8 buffer[5] = {0};
        MemoryOutput output(buffer);
        const char* data = "exact";
        size_t bytesWritten = output.write(data, 5);
        STD_INSIST(bytesWritten == 5);
        STD_INSIST(memcmp(buffer, "exact", 5) == 0);
    }

    STD_TEST(MultipleWrites) {
        u8 buffer[11] = {0};
        MemoryOutput output(buffer);
        const char* data1 = "hello";
        const char* data2 = " world";
        size_t bytesWritten1 = output.write(data1, 5);
        size_t bytesWritten2 = output.write(data2, 6);
        STD_INSIST(bytesWritten1 == 5);
        STD_INSIST(bytesWritten2 == 6);
        STD_INSIST(memcmp(buffer, "hello world", 11) == 0);
    }

    STD_TEST(IncrementalWrites) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        for (size_t i = 0; i < 10; ++i) {
            u8 byte = '0' + i;
            size_t bytesWritten = output.write(&byte, 1);
            STD_INSIST(bytesWritten == 1);
        }
        STD_INSIST(memcmp(buffer, "0123456789", 10) == 0);
    }

    STD_TEST(BinaryDataWrite) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        size_t bytesWritten = output.write(data, sizeof(data));
        STD_INSIST(bytesWritten == sizeof(data));
        STD_INSIST(memcmp(buffer, data, sizeof(data)) == 0);
    }

    STD_TEST(LargeWrite) {
        const size_t bufSize = 100000;
        u8* buffer = new u8[bufSize];
        memset(buffer, 0, bufSize);
        MemoryOutput output(buffer);
        u8* data = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            data[i] = (u8)(i % 256);
        }
        size_t bytesWritten = output.write(data, bufSize);
        STD_INSIST(bytesWritten == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(buffer[i] == (u8)(i % 256));
        }
        delete[] buffer;
        delete[] data;
    }

    STD_TEST(SingleByteWrite) {
        u8 buffer = 0;
        MemoryOutput output(&buffer);
        u8 data = 0xAB;
        size_t bytesWritten = output.write(&data, 1);
        STD_INSIST(bytesWritten == 1);
        STD_INSIST(buffer == 0xAB);
    }

    STD_TEST(NullBytesWrite) {
        u8 buffer[100];
        memset(buffer, 0xFF, 100);
        MemoryOutput output(buffer);
        u8 data[100] = {0};
        size_t bytesWritten = output.write(data, 100);
        STD_INSIST(bytesWritten == 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(buffer[i] == 0);
        }
    }

    STD_TEST(MaxBytesWrite) {
        u8 buffer[50];
        memset(buffer, 0, 50);
        MemoryOutput output(buffer);
        u8 data[50];
        for (size_t i = 0; i < 50; ++i) {
            data[i] = 0xFF;
        }
        size_t bytesWritten = output.write(data, 50);
        STD_INSIST(bytesWritten == 50);
        for (size_t i = 0; i < 50; ++i) {
            STD_INSIST(buffer[i] == 0xFF);
        }
    }

    STD_TEST(PointerAdvancement) {
        u8 buffer[20] = {0};
        MemoryOutput output(buffer);
        STD_INSIST(output.ptr == buffer);
        output.write("first", 5);
        STD_INSIST(output.ptr == buffer + 5);
        output.write("second", 6);
        STD_INSIST(output.ptr == buffer + 11);
        STD_INSIST(memcmp(buffer, "firstsecond", 11) == 0);
    }
}

STD_TEST_SUITE(MemoryOutputAsZeroCopy) {
    STD_TEST(ImbueAndCommit) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        size_t avail;
        void* chunk = output.imbue(&avail);
        STD_INSIST(chunk == buffer);
        STD_INSIST(avail == (size_t)-1);
        memcpy(chunk, "test", 4);
        output.commit(4);
        STD_INSIST(output.ptr == buffer + 4);
        STD_INSIST(memcmp(buffer, "test", 4) == 0);
    }

    STD_TEST(MultipleImbueCommit) {
        u8 buffer[20] = {0};
        MemoryOutput output(buffer);
        size_t avail;
        void* chunk1 = output.imbue(&avail);
        memcpy(chunk1, "hello", 5);
        output.commit(5);
        void* chunk2 = output.imbue(&avail);
        memcpy(chunk2, "world", 5);
        output.commit(5);
        STD_INSIST(output.ptr == buffer + 10);
        STD_INSIST(memcmp(buffer, "helloworld", 10) == 0);
    }

    STD_TEST(CommitZero) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        size_t avail;
        output.imbue(&avail);
        output.commit(0);
        STD_INSIST(output.ptr == buffer);
    }

    STD_TEST(NextAndCommit) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        void* chunk;
        size_t available = output.next(&chunk);
        STD_INSIST(chunk == buffer);
        STD_INSIST(available == (size_t)-1);
        memcpy(chunk, "data", 4);
        output.commit(4);
        STD_INSIST(output.ptr == buffer + 4);
        STD_INSIST(memcmp(buffer, "data", 4) == 0);
    }

    STD_TEST(ImbueWithLength) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        auto unbound = output.imbue(5);
        memcpy(unbound.ptr, "12345", 5);
        output.commit(5);
        STD_INSIST(output.ptr == buffer + 5);
        STD_INSIST(memcmp(buffer, "12345", 5) == 0);
    }

    STD_TEST(LargeImbue) {
        const size_t bufSize = 100000;
        u8* buffer = new u8[bufSize];
        memset(buffer, 0, bufSize);
        MemoryOutput output(buffer);
        size_t avail;
        void* chunk = output.imbue(&avail);
        for (size_t i = 0; i < bufSize; ++i) {
            ((u8*)chunk)[i] = (u8)(i % 256);
        }
        output.commit(bufSize);
        STD_INSIST(output.ptr == buffer + bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(buffer[i] == (u8)(i % 256));
        }
        delete[] buffer;
    }

    STD_TEST(BinaryDataZeroCopy) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        size_t avail;
        void* chunk = output.imbue(&avail);
        u8 data[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        memcpy(chunk, data, sizeof(data));
        output.commit(sizeof(data));
        STD_INSIST(memcmp(buffer, data, sizeof(data)) == 0);
    }

    STD_TEST(IncrementalCommits) {
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        size_t avail;
        for (size_t i = 0; i < 10; ++i) {
            void* chunk = output.imbue(&avail);
            ((u8*)chunk)[0] = '0' + i;
            output.commit(1);
        }
        STD_INSIST(memcmp(buffer, "0123456789", 10) == 0);
    }
}

STD_TEST_SUITE(MemoryOutputWithMemoryInput) {
    STD_TEST(CopyEmpty) {
        const char* inputData = "";
        MemoryInput input(inputData, 0);
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer);
    }

    STD_TEST(CopySmall) {
        const char* inputData = "hello";
        MemoryInput input(inputData, 5);
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + 5);
        STD_INSIST(memcmp(buffer, "hello", 5) == 0);
    }

    STD_TEST(CopyMedium) {
        const char* inputData = "Hello, World! This is a test message for copy function.";
        const size_t len = strlen(inputData);
        MemoryInput input(inputData, len);
        u8 buffer[100] = {0};
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + len);
        STD_INSIST(memcmp(buffer, inputData, len) == 0);
    }

    STD_TEST(CopyLarge) {
        const size_t bufSize = 100000;
        u8* inputData = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            inputData[i] = (u8)(i % 256);
        }
        MemoryInput input(inputData, bufSize);
        u8* buffer = new u8[bufSize];
        memset(buffer, 0, bufSize);
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(buffer[i] == (u8)(i % 256));
        }
        delete[] inputData;
        delete[] buffer;
    }

    STD_TEST(CopyBinaryData) {
        u8 inputData[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        MemoryInput input(inputData, sizeof(inputData));
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + sizeof(inputData));
        STD_INSIST(memcmp(buffer, inputData, sizeof(inputData)) == 0);
    }

    STD_TEST(CopySingleByte) {
        u8 inputData = 0xEF;
        MemoryInput input(&inputData, 1);
        u8 buffer = 0;
        MemoryOutput output(&buffer);
        copy(input, output);
        STD_INSIST(buffer == 0xEF);
    }

    STD_TEST(CopyNullBytes) {
        u8 inputData[100] = {0};
        MemoryInput input(inputData, 100);
        u8 buffer[100];
        memset(buffer, 0xFF, 100);
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(buffer[i] == 0);
        }
    }

    STD_TEST(CopyMaxBytes) {
        u8 inputData[50];
        for (size_t i = 0; i < 50; ++i) {
            inputData[i] = 0xFF;
        }
        MemoryInput input(inputData, 50);
        u8 buffer[50] = {0};
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + 50);
        for (size_t i = 0; i < 50; ++i) {
            STD_INSIST(buffer[i] == 0xFF);
        }
    }

    STD_TEST(CopyRepeatedPattern) {
        const size_t patternLen = 10;
        const size_t repeats = 500;
        const size_t totalSize = patternLen * repeats;
        u8* inputData = new u8[totalSize];
        for (size_t i = 0; i < totalSize; ++i) {
            inputData[i] = (u8)(i % patternLen);
        }
        MemoryInput input(inputData, totalSize);
        u8* buffer = new u8[totalSize];
        memset(buffer, 0, totalSize);
        MemoryOutput output(buffer);
        copy(input, output);
        STD_INSIST(output.ptr == buffer + totalSize);
        for (size_t i = 0; i < totalSize; ++i) {
            STD_INSIST(buffer[i] == (u8)(i % patternLen));
        }
        delete[] inputData;
        delete[] buffer;
    }

    STD_TEST(MultipleCopies) {
        const char* inputData1 = "first";
        const char* inputData2 = "second";
        MemoryInput input1(inputData1, 5);
        MemoryInput input2(inputData2, 6);
        u8 buffer1[10] = {0};
        u8 buffer2[10] = {0};
        MemoryOutput output1(buffer1);
        MemoryOutput output2(buffer2);
        copy(input1, output1);
        copy(input2, output2);
        STD_INSIST(output1.ptr == buffer1 + 5);
        STD_INSIST(output2.ptr == buffer2 + 6);
        STD_INSIST(memcmp(buffer1, "first", 5) == 0);
        STD_INSIST(memcmp(buffer2, "second", 6) == 0);
    }
}

STD_TEST_SUITE(MemoryOutputWithFDInput) {
    STD_TEST(CopyFromPipeEmpty) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        FDInput fdInput(readEnd);
        writeEnd.close();
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer);
    }

    STD_TEST(CopyFromPipeSmall) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        const char* testData = "hello";
        writeEnd.write(testData, 5);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + 5);
        STD_INSIST(memcmp(buffer, "hello", 5) == 0);
    }

    STD_TEST(CopyFromPipeMedium) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        const char* testData = "Hello, World! This is a test message.";
        const size_t len = strlen(testData);
        writeEnd.write(testData, len);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[100] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + len);
        STD_INSIST(memcmp(buffer, testData, len) == 0);
    }

    STD_TEST(CopyFromPipeLarge) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        const size_t bufSize = 8192;
        u8* testData = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            testData[i] = (u8)(i % 256);
        }
        writeEnd.write(testData, bufSize);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8* buffer = new u8[bufSize];
        memset(buffer, 0, bufSize);
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(buffer[i] == (u8)(i % 256));
        }
        delete[] testData;
        delete[] buffer;
    }

    STD_TEST(CopyFromPipeBinary) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        u8 testData[] = {0, 1, 2, 255, 254, 128, 127, 0, 0, 1};
        writeEnd.write(testData, sizeof(testData));
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[10] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + sizeof(testData));
        STD_INSIST(memcmp(buffer, testData, sizeof(testData)) == 0);
    }

    STD_TEST(CopyFromPipeSingleByte) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        u8 testData = 0xCD;
        writeEnd.write(&testData, 1);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer = 0;
        MemoryOutput output(&buffer);
        copy(fdInput, output);
        STD_INSIST(buffer == 0xCD);
    }

    STD_TEST(CopyFromPipeMultipleWrites) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        const char* part1 = "Hello";
        const char* part2 = " ";
        const char* part3 = "World";
        writeEnd.write(part1, strlen(part1));
        writeEnd.write(part2, strlen(part2));
        writeEnd.write(part3, strlen(part3));
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[20] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + 11);
        STD_INSIST(memcmp(buffer, "Hello World", 11) == 0);
    }

    STD_TEST(CopyFromPipeNullBytes) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        u8 testData[100] = {0};
        writeEnd.write(testData, 100);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[100];
        memset(buffer, 0xFF, 100);
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + 100);
        for (size_t i = 0; i < 100; ++i) {
            STD_INSIST(buffer[i] == 0);
        }
    }

    STD_TEST(CopyFromPipeMaxBytes) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        u8 testData[50];
        for (size_t i = 0; i < 50; ++i) {
            testData[i] = 0xFF;
        }
        writeEnd.write(testData, 50);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[50] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + 50);
        for (size_t i = 0; i < 50; ++i) {
            STD_INSIST(buffer[i] == 0xFF);
        }
    }

    STD_TEST(CopyFromPipeIncrementalPattern) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);
        const size_t bufSize = 1000;
        u8 testData[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            testData[i] = (u8)i;
        }
        writeEnd.write(testData, bufSize);
        writeEnd.close();
        FDInput fdInput(readEnd);
        u8 buffer[bufSize] = {0};
        MemoryOutput output(buffer);
        copy(fdInput, output);
        STD_INSIST(output.ptr == buffer + bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(buffer[i] == (u8)i);
        }
    }

    STD_TEST(MultiplePipes) {
        ScopedFD readEnd1, writeEnd1;
        ScopedFD readEnd2, writeEnd2;
        createPipeFD(readEnd1, writeEnd1);
        createPipeFD(readEnd2, writeEnd2);
        const char* msg1 = "Pipe1Data";
        const char* msg2 = "Pipe2Data";
        writeEnd1.write(msg1, strlen(msg1));
        writeEnd2.write(msg2, strlen(msg2));
        writeEnd1.close();
        writeEnd2.close();
        FDInput fdInput1(readEnd1);
        FDInput fdInput2(readEnd2);
        u8 buffer1[20] = {0};
        u8 buffer2[20] = {0};
        MemoryOutput output1(buffer1);
        MemoryOutput output2(buffer2);
        copy(fdInput1, output1);
        copy(fdInput2, output2);
        STD_INSIST(output1.ptr == buffer1 + strlen(msg1));
        STD_INSIST(output2.ptr == buffer2 + strlen(msg2));
        STD_INSIST(memcmp(buffer1, msg1, strlen(msg1)) == 0);
        STD_INSIST(memcmp(buffer2, msg2, strlen(msg2)) == 0);
    }
}
