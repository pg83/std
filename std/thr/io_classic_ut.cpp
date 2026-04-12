#include "coro.h"
#include "async.h"
#include "poller.h"
#include "poll_fd.h"
#include "channel.h"
#include "semaphore.h"
#include "io_classic.h"
#include "io_reactor.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/sys/mem_fd.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

#include <string.h>
#include <unistd.h>

using namespace stl;

namespace {
    static void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }

    int makeTmpFd() {
        return memFD("io_classic_ut");
    }

    struct Ctx {
        ObjPool::Ref pool;
        CoroExecutor* exec;
        IoReactor* io;

        Ctx()
            : pool(ObjPool::fromMemory())
            , exec(CoroExecutor::create(pool.mutPtr(), 4))
            , io(createPollIoReactor(pool.mutPtr(), exec, 4))
        {
        }
    };
}

STD_TEST_SUITE(IoClassicPoll) {
    STD_TEST(PipeReadWrite) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);
        int result = 0;

        exec->spawn([&] {
            u32 ready = io->poll({readEnd.get(), PollFlag::In}, UINT64_MAX);
            STD_INSIST(ready & PollFlag::In);
            char buf;
            readEnd.read(&buf, 1);
            result = buf;
        });

        exec->spawn([&] {
            char b = 42;
            writeEnd.write(&b, 1);
        });

        exec->join();
        STD_INSIST(result == 42);
    }

    STD_TEST(Timeout) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);

        auto f = async(exec, [&] {
            return io->poll({readEnd.get(), PollFlag::In}, 1000);
        });

        STD_INSIST(f.wait() == 0);
    }

    STD_TEST(MultiPipe) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        const int N = 8;
        ScopedFD readEnds[N], writeEnds[N];
        int order[N];
        int orderIdx = 0;

        for (int i = 0; i < N; i++) {
            createPipeFD(readEnds[i], writeEnds[i]);
        }

        for (int i = 0; i < N; i++) {
            exec->spawn([&, i] {
                u32 ready = io->poll({readEnds[i].get(), PollFlag::In}, UINT64_MAX);
                STD_INSIST(ready & PollFlag::In);
                char buf;
                readEnds[i].read(&buf, 1);
                order[stdAtomicAddAndFetch(&orderIdx, 1, MemoryOrder::Relaxed) - 1] = i;
            });
        }

        exec->spawn([&] {
            for (int i = N - 1; i >= 0; i--) {
                char b = 1;
                writeEnds[i].write(&b, 1);
            }
        });

        exec->join();
        STD_INSIST(orderIdx == N);
    }

    STD_TEST(SameFdMultiPoll) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);
        const int N = 40;
        int woken = 0;

        for (int i = 0; i < N; i++) {
            exec->spawn([&] {
                u32 ready = io->poll({readEnd.get(), PollFlag::In}, UINT64_MAX);
                STD_INSIST(ready & PollFlag::In);
                stdAtomicAddAndFetch(&woken, 1, MemoryOrder::Relaxed);
            });
        }

        exec->spawn([&] {
            char b = 1;
            writeEnd.write(&b, 1);
        });

        exec->join();
        STD_INSIST(woken == N);
    }

    STD_TEST(_ExceptionAcrossYield) {
        Ctx ctx;
        auto exec = ctx.exec;
        Channel ch(exec, 0);
        int caught = 0;

        exec->spawn([&] {
            try {
                try {
                    throw 42;
                } catch (...) {
                    void* dummy = nullptr;
                    ch.enqueue(&dummy);
                    throw;
                }
            } catch (int v) {
                if (v == 42) {
                    caught = 1;
                }
            }
        });

        exec->spawn([&] {
            void* v;
            ch.dequeue(&v);
            doW(1000000000);
        });

        exec->join();
        STD_INSIST(caught == 1);
    }

    STD_TEST(SleepZero) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;

        auto f = async(exec, [&] {
            io->poll({-1, 0}, monotonicNowUs());
            return true;
        });

        STD_INSIST(f.wait() == true);
    }

    STD_TEST(SleepZeroMultiple) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&] {
                io->poll({-1, 0}, monotonicNowUs());
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        exec->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(SleepZeroOrdering) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;

        auto f = async(exec, [&] {
            io->poll({-1, 0}, monotonicNowUs());
            return 2;
        });

        STD_INSIST(f.wait() == 2);
    }

    STD_TEST(PollMultiBasic) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);
        u32 result = 0;

        exec->spawn([&] {
            auto opool = ObjPool::fromMemory();
            auto p = io->createPoller(opool.mutPtr());

            p->arm({r1.get(), PollFlag::In});
            p->arm({r2.get(), PollFlag::In});

            p->wait([&](PollFD* ev) {
                result |= ev->flags;
            }, UINT64_MAX);
        });

        exec->spawn([&] {
            char b = 1;
            w1.write(&b, 1);
        });

        exec->join();
        STD_INSIST(result & PollFlag::In);
    }

    STD_TEST(PollMultiSecondFd) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);
        u32 result = 0;

        exec->spawn([&] {
            auto opool = ObjPool::fromMemory();
            auto p = io->createPoller(opool.mutPtr());

            p->arm({r1.get(), PollFlag::In});
            p->arm({r2.get(), PollFlag::In});

            p->wait([&](PollFD* ev) {
                result |= ev->flags;
            }, UINT64_MAX);
        });

        exec->spawn([&] {
            char b = 1;
            w2.write(&b, 1);
        });

        exec->join();
        STD_INSIST(result & PollFlag::In);
    }

    STD_TEST(PollMultiTimeout) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);

        auto f = async(exec, [&] {
            auto opool = ObjPool::fromMemory();
            auto p = io->createPoller(opool.mutPtr());

            p->arm({r1.get(), PollFlag::In});
            p->arm({r2.get(), PollFlag::In});

            size_t n = 0;

            p->wait([&](PollFD*) {
                ++n;
            }, 1);

            return n;
        });

        STD_INSIST(f.wait() == 0);
    }

    STD_TEST(PollMultiManyFds) {
        Ctx ctx;
        auto exec = ctx.exec;
        auto io = ctx.io;
        const int N = 8;
        ScopedFD readEnds[N], writeEnds[N];

        for (int i = 0; i < N; ++i) {
            createPipeFD(readEnds[i], writeEnds[i]);
        }

        u32 result = 0;

        exec->spawn([&] {
            auto opool = ObjPool::fromMemory();
            auto p = io->createPoller(opool.mutPtr());

            for (int i = 0; i < N; ++i) {
                p->arm({readEnds[i].get(), PollFlag::In});
            }

            p->wait([&](PollFD* ev) {
                result |= ev->flags;
            }, UINT64_MAX);
        });

        exec->spawn([&] {
            char b = 1;
            writeEnds[N / 2].write(&b, 1);
        });

        exec->join();
        STD_INSIST(result & PollFlag::In);
    }
}

STD_TEST_SUITE(IoClassicFS) {
    STD_TEST(PreadFile) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);
        auto io = createPollIoReactor(pool.mutPtr(), exec, 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello pread";
            ::write(fd, data, sizeof(data));

            char buf[32] = {};
            size_t n = 0;
            STD_INSIST(io->pread(fd, &n, buf, sizeof(buf), 0) == 0);

            STD_INSIST(n == sizeof(data));
            STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PwriteFile) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);
        auto io = createPollIoReactor(pool.mutPtr(), exec, 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello pwrite";
            size_t n = 0;
            STD_INSIST(io->pwrite(fd, &n, data, sizeof(data), 0) == 0);

            STD_INSIST(n == sizeof(data));

            char buf[32] = {};
            ::pread(fd, buf, sizeof(buf), 0);
            STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PreadPwriteRoundtrip) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);
        auto io = createPollIoReactor(pool.mutPtr(), exec, 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char msg[] = "roundtrip";
            size_t w = 0;
            STD_INSIST(io->pwrite(fd, &w, msg, sizeof(msg), 0) == 0);
            STD_INSIST(w == sizeof(msg));

            char buf[32] = {};
            size_t r = 0;
            STD_INSIST(io->pread(fd, &r, buf, sizeof(buf), 0) == 0);
            STD_INSIST(r == sizeof(msg));
            STD_INSIST(memcmp(buf, msg, sizeof(msg)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PreadOffset) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);
        auto io = createPollIoReactor(pool.mutPtr(), exec, 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "0123456789";
            ::write(fd, data, sizeof(data));

            char buf[4] = {};
            size_t n = 0;
            STD_INSIST(io->pread(fd, &n, buf, 4, 3) == 0);

            STD_INSIST(n == 4);
            STD_INSIST(memcmp(buf, "3456", 4) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(MultipleConcurrent) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto io = createPollIoReactor(pool.mutPtr(), exec, 4);

        exec->spawn([&] {
            int fd = makeTmpFd();
            const char data[] = "concurrent";
            ::write(fd, data, sizeof(data));

            Semaphore sem(0, exec);

            for (int i = 0; i < 8; ++i) {
                exec->spawn([&] {
                    char buf[16] = {};
                    size_t n = 0;
                    STD_INSIST(io->pread(fd, &n, buf, sizeof(buf), 0) == 0);
                    STD_INSIST(n == sizeof(data));
                    STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);
                    sem.post();
                });
            }

            for (int i = 0; i < 8; ++i) {
                sem.wait();
            }

            ::close(fd);
        });

        exec->join();
    }
}
