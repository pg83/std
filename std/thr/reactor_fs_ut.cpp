#include "reactor_fs.h"
#include "coro.h"
#include "semaphore.h"

#include <std/tst/ut.h>
#include <std/mem/obj_pool.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace stl;

namespace {
    struct TestFSRequest: public FSRequest {
        Semaphore sem;
        ssize_t result = 0;

        TestFSRequest(CoroExecutor* exec)
            : sem(0, exec)
        {
        }

        void complete(ssize_t res) noexcept override {
            result = res;
            sem.post();
        }

        void parkWith(Runable&& afterSuspend) noexcept override {
            afterSuspend.run();
            sem.wait();
        }
    };

    ssize_t doRead(FSReactorIface* reactor, CoroExecutor* exec,
                   int fd, void* buf, size_t len, off_t off)
    {
        struct iovec iov = {buf, len};
        TestFSRequest req(exec);
        req.iov = &iov;
        req.iovcnt = 1;
        req.offset = off;
        req.fd = fd;
        req.op = FSRequestOp::Read;
        reactor->submit(&req);
        return req.result;
    }

    ssize_t doWrite(FSReactorIface* reactor, CoroExecutor* exec,
                    int fd, const void* buf, size_t len, off_t off)
    {
        struct iovec iov = {(void*)buf, len};
        TestFSRequest req(exec);
        req.iov = &iov;
        req.iovcnt = 1;
        req.offset = off;
        req.fd = fd;
        req.op = FSRequestOp::Write;
        reactor->submit(&req);
        return req.result;
    }

    int makeTmpFd() {
        char path[] = "/tmp/reactor_fs_ut_XXXXXX";
        int fd = mkstemp(path);
        unlink(path);
        return fd;
    }
}

STD_TEST_SUITE(FSReactor) {
    STD_TEST(PreadFile) {
        auto exec = CoroExecutor::create(2);
        auto opool = ObjPool::fromMemory();
        auto* reactor = FSReactorIface::create(nullptr, opool.mutPtr());

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello reactor";
            ::write(fd, data, sizeof(data));

            char buf[32] = {};
            ssize_t n = doRead(reactor, exec.mutPtr(), fd, buf, sizeof(buf), 0);

            STD_INSIST(n == (ssize_t)sizeof(data));
            STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PwriteFile) {
        auto exec = CoroExecutor::create(2);
        auto opool = ObjPool::fromMemory();
        auto* reactor = FSReactorIface::create(nullptr, opool.mutPtr());

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "pwrite test";
            ssize_t n = doWrite(reactor, exec.mutPtr(), fd, data, sizeof(data), 0);

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
        auto opool = ObjPool::fromMemory();
        auto* reactor = FSReactorIface::create(nullptr, opool.mutPtr());

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char msg[] = "roundtrip";
            ssize_t written = doWrite(reactor, exec.mutPtr(), fd, msg, sizeof(msg), 16);
            STD_INSIST(written == (ssize_t)sizeof(msg));

            char buf[32] = {};
            ssize_t nread = doRead(reactor, exec.mutPtr(), fd, buf, sizeof(buf), 16);
            STD_INSIST(nread == (ssize_t)sizeof(msg));
            STD_INSIST(memcmp(buf, msg, sizeof(msg)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(MultipleIovecs) {
        auto exec = CoroExecutor::create(2);
        auto opool = ObjPool::fromMemory();
        auto* reactor = FSReactorIface::create(nullptr, opool.mutPtr());

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char part1[] = "hello";
            const char part2[] = " world";
            struct iovec wiov[2] = {
                {(void*)part1, sizeof(part1) - 1},
                {(void*)part2, sizeof(part2) - 1},
            };

            TestFSRequest wreq(exec.mutPtr());
            wreq.iov = wiov;
            wreq.iovcnt = 2;
            wreq.offset = 0;
            wreq.fd = fd;
            wreq.op = FSRequestOp::Write;
            reactor->submit(&wreq);
            STD_INSIST(wreq.result == 11);

            char buf[16] = {};
            struct iovec riov = {buf, sizeof(buf)};
            TestFSRequest rreq(exec.mutPtr());
            rreq.iov = &riov;
            rreq.iovcnt = 1;
            rreq.offset = 0;
            rreq.fd = fd;
            rreq.op = FSRequestOp::Read;
            reactor->submit(&rreq);
            STD_INSIST(rreq.result == 11);
            STD_INSIST(memcmp(buf, "hello world", 11) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(MultipleConcurrentPreads) {
        auto exec = CoroExecutor::create(4);
        auto opool = ObjPool::fromMemory();
        auto* reactor = FSReactorIface::create(nullptr, opool.mutPtr());

        constexpr int N = 8;
        int doneCount = 0;

        int fd = makeTmpFd();
        char fileData[64] = "concurrent read data for testing";
        ::write(fd, fileData, sizeof(fileData));

        for (int i = 0; i < N; ++i) {
            exec->spawn([&] {
                char buf[64] = {};
                ssize_t n = doRead(reactor, exec.mutPtr(), fd, buf, sizeof(buf), 0);
                STD_INSIST(n == (ssize_t)sizeof(fileData));
                STD_INSIST(memcmp(buf, fileData, sizeof(fileData)) == 0);

                if (++doneCount == N) {
                    ::close(fd);
                }
            });
        }

        exec->join();
    }
}
