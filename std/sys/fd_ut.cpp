#include "fd.h"

#include <std/tst/ut.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <cstring>
#include <cstdlib>

using namespace Std;

STD_TEST_SUITE(FD) {
    STD_TEST(ReadWriteMemFd) {
        int fd = memfd_create("test", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);

        const char* testData = "Hello, World!";
        size_t len = strlen(testData);

        size_t written = fdWrapper.write(testData, len);
        STD_INSIST(written == len);

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        size_t readBytes = fdWrapper.read(readBuf, len);
        STD_INSIST(readBytes == len);
        STD_INSIST(memcmp(readBuf, testData, len) == 0);

        close(fd);
    }

    STD_TEST(ReadEmptyMemFd) {
        int fd = memfd_create("test_empty", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);

        char readBuf[64];
        size_t readBytes = fdWrapper.read(readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == 0);

        close(fd);
    }

    STD_TEST(WriteMultipleChunksMemFd) {
        int fd = memfd_create("test_chunks", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);

        const char* chunk1 = "First ";
        const char* chunk2 = "Second ";
        const char* chunk3 = "Third";

        fdWrapper.write(chunk1, strlen(chunk1));
        fdWrapper.write(chunk2, strlen(chunk2));
        fdWrapper.write(chunk3, strlen(chunk3));

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        size_t totalRead = fdWrapper.read(readBuf, sizeof(readBuf));

        STD_INSIST(totalRead == strlen(chunk1) + strlen(chunk2) + strlen(chunk3));
        STD_INSIST(strcmp(readBuf, "First Second Third") == 0);

        close(fd);
    }

    STD_TEST(WriteVectorMemFd) {
        int fd = memfd_create("test_writev", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);

        const char* part1 = "Hello";
        const char* part2 = " ";
        const char* part3 = "World";

        iovec iov[3];
        iov[0].iov_base = (void*)part1;
        iov[0].iov_len = strlen(part1);
        iov[1].iov_base = (void*)part2;
        iov[1].iov_len = strlen(part2);
        iov[2].iov_base = (void*)part3;
        iov[2].iov_len = strlen(part3);

        size_t written = fdWrapper.writeV(iov, 3);
        STD_INSIST(written == strlen(part1) + strlen(part2) + strlen(part3));

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        size_t readBytes = fdWrapper.read(readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, "Hello World") == 0);

        close(fd);
    }

    STD_TEST(FsyncMemFd) {
        int fd = memfd_create("test_fsync", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);

        const char* testData = "Data to sync";
        fdWrapper.write(testData, strlen(testData));

        fdWrapper.fsync();

        close(fd);
    }

    STD_TEST(PipeReadWrite) {
        int pipefd[2];
        int result = pipe(pipefd);
        STD_INSIST(result == 0);

        FD readEnd(pipefd[0]);
        FD writeEnd(pipefd[1]);

        const char* testData = "Pipe test data";
        size_t len = strlen(testData);

        size_t written = writeEnd.write(testData, len);
        STD_INSIST(written == len);

        char readBuf[64] = {0};
        size_t readBytes = readEnd.read(readBuf, len);
        STD_INSIST(readBytes == len);
        STD_INSIST(memcmp(readBuf, testData, len) == 0);

        close(pipefd[0]);
        close(pipefd[1]);
    }

    STD_TEST(PipeMultipleWrites) {
        int pipefd[2];
        pipe(pipefd);

        FD readEnd(pipefd[0]);
        FD writeEnd(pipefd[1]);

        const char* msg1 = "First";
        const char* msg2 = "Second";

        writeEnd.write(msg1, strlen(msg1));
        writeEnd.write(msg2, strlen(msg2));

        char readBuf[64] = {0};
        size_t totalLen = strlen(msg1) + strlen(msg2);
        size_t readBytes = readEnd.read(readBuf, totalLen);

        STD_INSIST(readBytes == totalLen);
        STD_INSIST(memcmp(readBuf, "FirstSecond", totalLen) == 0);

        close(pipefd[0]);
        close(pipefd[1]);
    }

    STD_TEST(PipeWriteV) {
        int pipefd[2];
        pipe(pipefd);

        FD readEnd(pipefd[0]);
        FD writeEnd(pipefd[1]);

        const char* part1 = "A";
        const char* part2 = "B";
        const char* part3 = "C";

        iovec iov[3];
        iov[0].iov_base = (void*)part1;
        iov[0].iov_len = strlen(part1);
        iov[1].iov_base = (void*)part2;
        iov[1].iov_len = strlen(part2);
        iov[2].iov_base = (void*)part3;
        iov[2].iov_len = strlen(part3);

        size_t written = writeEnd.writeV(iov, 3);
        STD_INSIST(written == 3);

        char readBuf[64] = {0};
        size_t readBytes = readEnd.read(readBuf, 3);
        STD_INSIST(strcmp(readBuf, "ABC") == 0);

        close(pipefd[0]);
        close(pipefd[1]);
    }

    STD_TEST(LargeDataMemFd) {
        int fd = memfd_create("test_large", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);

        const size_t bufSize = 65536;
        u8* writeBuf = (u8*)malloc(bufSize);
        for (size_t i = 0; i < bufSize; ++i) {
            writeBuf[i] = (u8)(i % 256);
        }

        size_t written = fdWrapper.write(writeBuf, bufSize);
        STD_INSIST(written == bufSize);

        lseek(fd, 0, SEEK_SET);

        u8* readBuf = (u8*)malloc(bufSize);
        size_t readBytes = fdWrapper.read(readBuf, bufSize);
        STD_INSIST(readBytes == bufSize);

        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(readBuf[i] == writeBuf[i]);
        }

        free(writeBuf);
        free(readBuf);
        close(fd);
    }
}

STD_TEST_SUITE(ScopedFD) {
    STD_TEST(AutoCloseOnDestroy) {
        int fd = memfd_create("test_scoped", 0);
        STD_INSIST(fd >= 0);

        {
            ScopedFD scoped(fd);
            const char* testData = "Scoped test";
            scoped.write(testData, strlen(testData));
        }
    }

    STD_TEST(ReadWriteScoped) {
        int fd = memfd_create("test_scoped_rw", 0);
        STD_INSIST(fd >= 0);

        ScopedFD scoped(fd);

        const char* testData = "Scoped read/write";
        size_t len = strlen(testData);

        scoped.write(testData, len);
        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        size_t readBytes = scoped.read(readBuf, len);
        STD_INSIST(readBytes == len);
        STD_INSIST(memcmp(readBuf, testData, len) == 0);
    }

    STD_TEST(ScopedPipe) {
        int pipefd[2];
        pipe(pipefd);

        {
            ScopedFD readEnd(pipefd[0]);
            ScopedFD writeEnd(pipefd[1]);

            const char* testData = "Scoped pipe";
            writeEnd.write(testData, strlen(testData));

            char readBuf[64] = {0};
            readEnd.read(readBuf, strlen(testData));
            STD_INSIST(strcmp(readBuf, testData) == 0);
        }
    }

    STD_TEST(ScopedWriteV) {
        int fd = memfd_create("test_scoped_writev", 0);
        STD_INSIST(fd >= 0);

        ScopedFD scoped(fd);

        const char* p1 = "X";
        const char* p2 = "Y";
        const char* p3 = "Z";

        iovec iov[3];
        iov[0].iov_base = (void*)p1;
        iov[0].iov_len = 1;
        iov[1].iov_base = (void*)p2;
        iov[1].iov_len = 1;
        iov[2].iov_base = (void*)p3;
        iov[2].iov_len = 1;

        size_t written = scoped.writeV(iov, 3);
        STD_INSIST(written == 3);

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        scoped.read(readBuf, 3);
        STD_INSIST(strcmp(readBuf, "XYZ") == 0);
    }

    STD_TEST(ScopedFsync) {
        int fd = memfd_create("test_scoped_fsync", 0);
        STD_INSIST(fd >= 0);

        ScopedFD scoped(fd);

        const char* testData = "Fsync data";
        scoped.write(testData, strlen(testData));
        scoped.fsync();
    }
}
