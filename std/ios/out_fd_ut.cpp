#include "out_fd.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>
#include <std/str/view.h>
#include <std/lib/buffer.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <cstring>

using namespace Std;

STD_TEST_SUITE(FDRegular) {
    STD_TEST(BasicWrite) {
        int fd = memfd_create("test_basic", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        const char* testData = "Hello, FDRegular!";
        size_t len = strlen(testData);

        fdRegular.write(testData, len);
        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == (ssize_t)len);
        STD_INSIST(memcmp(readBuf, testData, len) == 0);

        close(fd);
    }

    STD_TEST(MultipleWrites) {
        int fd = memfd_create("test_multiple", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        const char* chunk1 = "First ";
        const char* chunk2 = "Second ";
        const char* chunk3 = "Third";

        fdRegular.write(chunk1, strlen(chunk1));
        fdRegular.write(chunk2, strlen(chunk2));
        fdRegular.write(chunk3, strlen(chunk3));
        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        size_t totalLen = strlen(chunk1) + strlen(chunk2) + strlen(chunk3);
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == (ssize_t)totalLen);
        STD_INSIST(strcmp(readBuf, "First Second Third") == 0);

        close(fd);
    }

    STD_TEST(WriteV) {
        int fd = memfd_create("test_writev", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

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

        size_t written = fdRegular.writeV(iov, 3);
        size_t expectedLen = strlen(part1) + strlen(part2) + strlen(part3);
        STD_INSIST(written == expectedLen);

        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, "Hello World") == 0);

        close(fd);
    }

    STD_TEST(Flush) {
        int fd = memfd_create("test_flush", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        const char* testData = "Flush test data";
        fdRegular.write(testData, strlen(testData));

        fdRegular.flush();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == (ssize_t)strlen(testData));
        STD_INSIST(strcmp(readBuf, testData) == 0);

        close(fd);
    }

    STD_TEST(MultipleFlushes) {
        int fd = memfd_create("test_flushes", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        const char* data1 = "Part1";
        const char* data2 = "Part2";
        const char* data3 = "Part3";

        fdRegular.write(data1, strlen(data1));
        fdRegular.flush();

        fdRegular.write(data2, strlen(data2));
        fdRegular.flush();

        fdRegular.write(data3, strlen(data3));
        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        size_t totalLen = strlen(data1) + strlen(data2) + strlen(data3);
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == (ssize_t)totalLen);
        STD_INSIST(strcmp(readBuf, "Part1Part2Part3") == 0);

        close(fd);
    }

    STD_TEST(LargeData) {
        int fd = memfd_create("test_large", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        const size_t bufSize = 65536;
        u8* writeBuf = new u8[bufSize];
        for (size_t i = 0; i < bufSize; ++i) {
            writeBuf[i] = (u8)(i % 256);
        }

        fdRegular.write(writeBuf, bufSize);
        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        u8* readBuf = new u8[bufSize];
        ssize_t readBytes = ::read(fd, readBuf, bufSize);
        STD_INSIST(readBytes == (ssize_t)bufSize);

        for (size_t i = 0; i < bufSize; ++i) {
            STD_INSIST(readBuf[i] == writeBuf[i]);
        }

        delete[] writeBuf;
        delete[] readBuf;
        close(fd);
    }

    STD_TEST(EmptyWrite) {
        int fd = memfd_create("test_empty", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64];
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(readBytes == 0);

        close(fd);
    }

    STD_TEST(WriteStringView) {
        int fd = memfd_create("test_strview", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        StringView views[3];
        views[0] = StringView(u8"Hello");
        views[1] = StringView(u8" ");
        views[2] = StringView(u8"World");

        size_t written = fdRegular.writeV(views, 3);
        size_t expectedLen = views[0].length() + views[1].length() + views[2].length();
        STD_INSIST(written == expectedLen);

        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, "Hello World") == 0);

        close(fd);
    }

    STD_TEST(HintCheck) {
        int fd = memfd_create("test_hint", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        size_t hintSize = 0;
        bool hasHint = fdRegular.hint(&hintSize);
        STD_INSIST(hasHint);
        STD_INSIST(hintSize == (1 << 16));

        close(fd);
    }

    STD_TEST(FlushWithoutData) {
        int fd = memfd_create("test_flush_empty", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        fdRegular.flush();
        fdRegular.finish();

        close(fd);
    }

    STD_TEST(WriteAfterFlush) {
        int fd = memfd_create("test_write_after_flush", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        const char* data1 = "Before";
        const char* data2 = "After";

        fdRegular.write(data1, strlen(data1));
        fdRegular.flush();

        fdRegular.write(data2, strlen(data2));
        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, "BeforeAfter") == 0);

        close(fd);
    }

    STD_TEST(WriteMixedSizes) {
        int fd = memfd_create("test_mixed", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDRegular fdRegular(fdWrapper);

        fdRegular.write("a", 1);
        fdRegular.write("bc", 2);
        fdRegular.write("def", 3);
        fdRegular.write("ghij", 4);
        fdRegular.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, "abcdefghij") == 0);

        close(fd);
    }
}

STD_TEST_SUITE(FDCharacter) {
    STD_TEST(HintCheck) {
        int fd = memfd_create("test_char_hint", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDCharacter fdChar(fdWrapper);

        size_t hintSize = 0;
        bool hasHint = fdChar.hint(&hintSize);
        STD_INSIST(hasHint);
        STD_INSIST(hintSize == (1 << 10));

        close(fd);
    }

    STD_TEST(BasicWrite) {
        int fd = memfd_create("test_char_write", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDCharacter fdChar(fdWrapper);

        const char* testData = "Character device test";
        fdChar.write(testData, strlen(testData));
        fdChar.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, testData) == 0);

        close(fd);
    }
}

STD_TEST_SUITE(FDPipe) {
    STD_TEST(HintCheck) {
        int fd = memfd_create("test_pipe_hint", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDPipe fdPipe(fdWrapper);

        size_t hintSize = 0;
        bool hasHint = fdPipe.hint(&hintSize);
        STD_INSIST(hasHint);
        STD_INSIST(hintSize == (1 << 12));

        close(fd);
    }

    STD_TEST(BasicWrite) {
        int fd = memfd_create("test_pipe_write", 0);
        STD_INSIST(fd >= 0);

        FD fdWrapper(fd);
        FDPipe fdPipe(fdWrapper);

        const char* testData = "Pipe test";
        fdPipe.write(testData, strlen(testData));
        fdPipe.finish();

        lseek(fd, 0, SEEK_SET);

        char readBuf[64] = {0};
        ssize_t readBytes = ::read(fd, readBuf, sizeof(readBuf));
        STD_INSIST(strcmp(readBuf, testData) == 0);

        close(fd);
    }
}
