#include "in_fd.h"
#include "out_fd.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>

#include <sys/uio.h>
#include <cstring>

using namespace Std;

STD_TEST_SUITE(FDInput) {
    STD_TEST(BasicRead) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* testData = "Hello, FDInput!";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        char readBuf[64] = {0};
        size_t readBytes = fdInput.readP(readBuf, strlen(testData));
        STD_INSIST(readBytes == strlen(testData));
        STD_INSIST(strcmp(readBuf, testData) == 0);
    }

    STD_TEST(ReadExact) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* testData = "Exact read test";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        char readBuf[64] = {0};
        fdInput.read(readBuf, strlen(testData));
        STD_INSIST(strcmp(readBuf, testData) == 0);
    }

    STD_TEST(MultipleReads) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* chunk1 = "First ";
        const char* chunk2 = "Second ";
        const char* chunk3 = "Third";

        fdPipe.write(chunk1, strlen(chunk1));
        fdPipe.write(chunk2, strlen(chunk2));
        fdPipe.write(chunk3, strlen(chunk3));
        fdPipe.finish();

        char readBuf1[16] = {0};
        char readBuf2[16] = {0};
        char readBuf3[16] = {0};

        size_t read1 = fdInput.readP(readBuf1, strlen(chunk1));
        size_t read2 = fdInput.readP(readBuf2, strlen(chunk2));
        size_t read3 = fdInput.readP(readBuf3, strlen(chunk3));

        STD_INSIST(read1 == strlen(chunk1));
        STD_INSIST(read2 == strlen(chunk2));
        STD_INSIST(read3 == strlen(chunk3));
        STD_INSIST(strcmp(readBuf1, chunk1) == 0);
        STD_INSIST(strcmp(readBuf2, chunk2) == 0);
        STD_INSIST(strcmp(readBuf3, chunk3) == 0);
    }

    STD_TEST(ReadLargeData) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const size_t bufSize = 8192;
        u8* writeBuf = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            writeBuf[i] = (u8)(i % 256);
        }

        fdPipe.write(writeBuf, bufSize);
        fdPipe.finish();

        u8* readBuf = new u8[bufSize];
        size_t totalRead = 0;
        while (totalRead < bufSize) {
            size_t bytesRead = fdInput.readP(readBuf + totalRead, bufSize - totalRead);
            if (bytesRead == 0) {
                break;
            }
            totalRead += bytesRead;
        }

        STD_INSIST(totalRead == bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(readBuf[i] == writeBuf[i]);
        }

        delete[] writeBuf;
        delete[] readBuf;
    }

    STD_TEST(ReadSmallChunks) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* testData = "ABCDEFGHIJ";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        char readBuf[16] = {0};
        size_t totalRead = 0;
        for (size_t i = 0; i < 5; ++i) {
            size_t bytesRead = fdInput.readP(readBuf + totalRead, 2);
            STD_INSIST(bytesRead == 2);
            totalRead += bytesRead;
        }

        STD_INSIST(strcmp(readBuf, testData) == 0);
    }

    STD_TEST(ReadEmpty) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        fdPipe.finish();
        writeEnd.close();

        char readBuf[64];
        size_t readBytes = fdInput.readP(readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == 0);
    }

    STD_TEST(ReadPartialThenRest) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* testData = "HelloWorld";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        char readBuf1[6] = {0};
        size_t read1 = fdInput.readP(readBuf1, 5);
        STD_INSIST(read1 == 5);
        STD_INSIST(strcmp(readBuf1, "Hello") == 0);

        char readBuf2[6] = {0};
        size_t read2 = fdInput.readP(readBuf2, 5);
        STD_INSIST(read2 == 5);
        STD_INSIST(strcmp(readBuf2, "World") == 0);
    }

    STD_TEST(ReadWithWriteV) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* part1 = "Alpha";
        const char* part2 = "Beta";
        const char* part3 = "Gamma";

        iovec iov[3];
        iov[0].iov_base = (void*)part1;
        iov[0].iov_len = strlen(part1);
        iov[1].iov_base = (void*)part2;
        iov[1].iov_len = strlen(part2);
        iov[2].iov_base = (void*)part3;
        iov[2].iov_len = strlen(part3);

        fdPipe.writeV(iov, 3);
        fdPipe.finish();

        size_t totalLen = strlen(part1) + strlen(part2) + strlen(part3);
        char readBuf[64] = {0};
        size_t readBytes = fdInput.readP(readBuf, totalLen);
        STD_INSIST(readBytes == totalLen);
        STD_INSIST(strcmp(readBuf, "AlphaBetaGamma") == 0);
    }

    STD_TEST(ReadByte) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* testData = "ABC";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        char byte1, byte2, byte3;
        size_t read1 = fdInput.readP(&byte1, 1);
        size_t read2 = fdInput.readP(&byte2, 1);
        size_t read3 = fdInput.readP(&byte3, 1);

        STD_INSIST(read1 == 1 && byte1 == 'A');
        STD_INSIST(read2 == 1 && byte2 == 'B');
        STD_INSIST(read3 == 1 && byte3 == 'C');
    }

    STD_TEST(ReadWithFlush) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* data1 = "Part1";
        const char* data2 = "Part2";

        fdPipe.write(data1, strlen(data1));
        fdPipe.flush();

        char readBuf1[16] = {0};
        size_t read1 = fdInput.readP(readBuf1, strlen(data1));
        STD_INSIST(read1 == strlen(data1));
        STD_INSIST(strcmp(readBuf1, data1) == 0);

        fdPipe.write(data2, strlen(data2));
        fdPipe.finish();

        char readBuf2[16] = {0};
        size_t read2 = fdInput.readP(readBuf2, strlen(data2));
        STD_INSIST(read2 == strlen(data2));
        STD_INSIST(strcmp(readBuf2, data2) == 0);
    }

    STD_TEST(ReadMixedSizes) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        fdPipe.write("a", 1);
        fdPipe.write("bc", 2);
        fdPipe.write("def", 3);
        fdPipe.write("ghij", 4);
        fdPipe.finish();

        char readBuf[64] = {0};
        size_t totalRead = 0;
        size_t bytesRead;

        bytesRead = fdInput.readP(readBuf + totalRead, 2);
        totalRead += bytesRead;

        bytesRead = fdInput.readP(readBuf + totalRead, 3);
        totalRead += bytesRead;

        bytesRead = fdInput.readP(readBuf + totalRead, 5);
        totalRead += bytesRead;

        STD_INSIST(totalRead == 10);
        STD_INSIST(strcmp(readBuf, "abcdefghij") == 0);
    }

    STD_TEST(MultiplePipes) {
        ScopedFD readEnd1, writeEnd1;
        ScopedFD readEnd2, writeEnd2;
        createPipeFD(readEnd1, writeEnd1);
        createPipeFD(readEnd2, writeEnd2);

        FDPipe fdPipe1(writeEnd1);
        FDPipe fdPipe2(writeEnd2);
        FDInput fdInput1(readEnd1);
        FDInput fdInput2(readEnd2);

        const char* msg1 = "Message1";
        const char* msg2 = "Message2";

        fdPipe1.write(msg1, strlen(msg1));
        fdPipe2.write(msg2, strlen(msg2));

        fdPipe1.finish();
        fdPipe2.finish();

        char readBuf1[64] = {0};
        char readBuf2[64] = {0};

        size_t read1 = fdInput1.readP(readBuf1, strlen(msg1));
        size_t read2 = fdInput2.readP(readBuf2, strlen(msg2));

        STD_INSIST(read1 == strlen(msg1));
        STD_INSIST(read2 == strlen(msg2));
        STD_INSIST(strcmp(readBuf1, msg1) == 0);
        STD_INSIST(strcmp(readBuf2, msg2) == 0);
    }

    STD_TEST(ReadBinaryData) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        u8 binaryData[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        fdPipe.write(binaryData, 10);
        fdPipe.finish();

        u8 readBuf[10] = {0};
        size_t readBytes = fdInput.readP(readBuf, 10);
        STD_INSIST(readBytes == 10);

        for (size_t i = 0; i < 10; ++i) {
            STD_INSIST(readBuf[i] == binaryData[i]);
        }
    }

    STD_TEST(ReadZeroBytes) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const char* testData = "Test";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        char readBuf[64] = {0};
        size_t readBytes = fdInput.readP(readBuf, 0);
        STD_INSIST(readBytes == 0);
    }

    STD_TEST(ReadIncrementalPattern) {
        ScopedFD readEnd;
        ScopedFD writeEnd;
        createPipeFD(readEnd, writeEnd);

        FDPipe fdPipe(writeEnd);
        FDInput fdInput(readEnd);

        const size_t bufSize = 100;
        u8 writeBuf[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            writeBuf[i] = (u8)i;
        }

        fdPipe.write(writeBuf, bufSize);
        fdPipe.finish();

        u8 readBuf[bufSize] = {0};
        size_t readBytes = fdInput.readP(readBuf, bufSize);
        STD_INSIST(readBytes == bufSize);

        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(readBuf[i] == (u8)i);
        }
    }
}
