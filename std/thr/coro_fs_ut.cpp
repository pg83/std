#include "coro.h"

#include <std/tst/ut.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace stl;

namespace {
    int makeTmpFd() {
        char path[] = "/tmp/coro_fs_ut_XXXXXX";
        int fd = mkstemp(path);
        unlink(path);
        return fd;
    }
}

STD_TEST_SUITE(CoroExecutorFS) {
    STD_TEST(PreadFile) {
        auto exec = CoroExecutor::create(2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello pread";
            ::write(fd, data, sizeof(data));

            char buf[32] = {};
            ssize_t n = exec->pread(fd, buf, sizeof(buf), 0);

            STD_INSIST(n == (ssize_t)sizeof(data));
            STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PwriteFile) {
        auto exec = CoroExecutor::create(2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello pwrite";
            ssize_t n = exec->pwrite(fd, data, sizeof(data), 0);

            STD_INSIST(n == (ssize_t)sizeof(data));

            char buf[32] = {};
            ::pread(fd, buf, sizeof(buf), 0);
            STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PreadPwriteRoundtrip) {
        auto exec = CoroExecutor::create(2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char msg[] = "roundtrip";
            ssize_t w = exec->pwrite(fd, msg, sizeof(msg), 0);
            STD_INSIST(w == (ssize_t)sizeof(msg));

            char buf[32] = {};
            ssize_t r = exec->pread(fd, buf, sizeof(buf), 0);
            STD_INSIST(r == (ssize_t)sizeof(msg));
            STD_INSIST(memcmp(buf, msg, sizeof(msg)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PreadOffset) {
        auto exec = CoroExecutor::create(2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "0123456789";
            ::write(fd, data, sizeof(data));

            char buf[4] = {};
            ssize_t n = exec->pread(fd, buf, 4, 3);

            STD_INSIST(n == 4);
            STD_INSIST(memcmp(buf, "3456", 4) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(MultipleConcurrent) {
        auto exec = CoroExecutor::create(4);
        int done = 0;

        exec->spawn([&] {
            int fd = makeTmpFd();
            const char data[] = "concurrent";
            ::write(fd, data, sizeof(data));

            for (int i = 0; i < 8; ++i) {
                exec->spawn([&] {
                    char buf[16] = {};
                    ssize_t n = exec->pread(fd, buf, sizeof(buf), 0);
                    STD_INSIST(n == (ssize_t)sizeof(data));
                    STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

                    if (++done == 8) {
                        ::close(fd);
                    }
                });
            }
        });

        exec->join();
    }
}
