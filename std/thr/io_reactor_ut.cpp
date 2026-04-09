#include "coro.h"
#include "async.h"
#include "poll_fd.h"
#include "channel.h"
#include "semaphore.h"
#include "io_reactor.h"
#include "coro_config.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/sys/mem_fd.h>
#include <std/sys/atomic.h>
#include <std/lib/visitor.h>
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
        return memFD("io_reactor_ut");
    }
}

STD_TEST_SUITE(IoReactorPoll) {
    STD_TEST(PipeReadWrite) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);
        int result = 0;

        exec->spawn([&] {
            auto io = exec->io();
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);

        auto f = async(exec, [&] {
            return exec->io()->poll({readEnd.get(), PollFlag::In}, 1000);
        });

        STD_INSIST(f.wait() == 0);
    }

    STD_TEST(MultiPipe) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 8;
        ScopedFD readEnds[N], writeEnds[N];
        int order[N];
        int orderIdx = 0;

        for (int i = 0; i < N; i++) {
            createPipeFD(readEnds[i], writeEnds[i]);
        }

        for (int i = 0; i < N; i++) {
            exec->spawn([&, i] {
                auto io = exec->io();
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD readEnd, writeEnd;
        createPipeFD(readEnd, writeEnd);
        const int N = 40;
        int woken = 0;

        for (int i = 0; i < N; i++) {
            exec->spawn([&] {
                u32 ready = exec->io()->poll({readEnd.get(), PollFlag::In}, UINT64_MAX);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [&] {
            exec->io()->poll({-1, 0}, monotonicNowUs());
            return true;
        });

        STD_INSIST(f.wait() == true);
    }

    STD_TEST(SleepZeroMultiple) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 100; ++i) {
            exec->spawn([&] {
                exec->io()->poll({-1, 0}, monotonicNowUs());
                stdAtomicAddAndFetch(&counter, 1, MemoryOrder::Relaxed);
            });
        }

        exec->join();
        STD_INSIST(counter == 100);
    }

    STD_TEST(SleepZeroOrdering) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto f = async(exec, [&] {
            exec->io()->poll({-1, 0}, monotonicNowUs());
            return 2;
        });

        STD_INSIST(f.wait() == 2);
    }

    STD_TEST(PollMultiBasic) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);
        u32 result = 0;

        exec->spawn([&] {
            auto opool = ObjPool::fromMemory();
            PollFD in[] = {{r1.get(), PollFlag::In}, {r2.get(), PollFlag::In}};
            auto g = exec->io()->createPollGroup(opool.mutPtr(), in, 2);

            // clang-format off
            exec->io()->poll(g, makeVisitor([&](void* ptr) {
                result |= ((PollFD*)ptr)->flags;
            }), UINT64_MAX);
            // clang-format on
        });

        exec->spawn([&] {
            char b = 1;
            w1.write(&b, 1);
        });

        exec->join();
        STD_INSIST(result & PollFlag::In);
    }

    STD_TEST(PollMultiSecondFd) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);
        u32 result = 0;

        exec->spawn([&] {
            auto opool = ObjPool::fromMemory();
            PollFD in[] = {{r1.get(), PollFlag::In}, {r2.get(), PollFlag::In}};
            auto g = exec->io()->createPollGroup(opool.mutPtr(), in, 2);

            // clang-format off
            exec->io()->poll(g, makeVisitor([&](void* ptr) {
                result |= ((PollFD*)ptr)->flags;
            }), UINT64_MAX);
            // clang-format on
        });

        exec->spawn([&] {
            char b = 1;
            w2.write(&b, 1);
        });

        exec->join();
        STD_INSIST(result & PollFlag::In);
    }

    STD_TEST(PollMultiTimeout) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);

        auto f = async(exec, [&] {
            auto opool = ObjPool::fromMemory();
            PollFD in[] = {{r1.get(), PollFlag::In}, {r2.get(), PollFlag::In}};
            auto g = exec->io()->createPollGroup(opool.mutPtr(), in, 2);
            size_t n = 0;

            // clang-format off
            exec->io()->poll(g, makeVisitor([&](void*) {
                ++n;
            }), 1);
            // clang-format on

            return n;
        });

        STD_INSIST(f.wait() == 0);
    }

    STD_TEST(PollMultiManyFds) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 8;
        ScopedFD readEnds[N], writeEnds[N];

        for (int i = 0; i < N; ++i) {
            createPipeFD(readEnds[i], writeEnds[i]);
        }

        u32 result = 0;

        exec->spawn([&] {
            auto opool = ObjPool::fromMemory();
            PollFD in[N];

            for (int i = 0; i < N; ++i) {
                in[i] = {readEnds[i].get(), PollFlag::In};
            }

            auto g = exec->io()->createPollGroup(opool.mutPtr(), in, N);

            // clang-format off
            exec->io()->poll(g, makeVisitor([&](void* ptr) {
                result |= ((PollFD*)ptr)->flags;
            }), UINT64_MAX);
            // clang-format on
        });

        exec->spawn([&] {
            char b = 1;
            writeEnds[N / 2].write(&b, 1);
        });

        exec->join();
        STD_INSIST(result & PollFlag::In);
    }
}

STD_TEST_SUITE(IoReactorFS) {
    STD_TEST(PreadFile) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello pread";
            ::write(fd, data, sizeof(data));

            char buf[32] = {};
            size_t n = 0;
            STD_INSIST(exec->io()->pread(fd, &n, buf, sizeof(buf), 0) == 0);

            STD_INSIST(n == sizeof(data));
            STD_INSIST(memcmp(buf, data, sizeof(data)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PwriteFile) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "hello pwrite";
            size_t n = 0;
            STD_INSIST(exec->io()->pwrite(fd, &n, data, sizeof(data), 0) == 0);

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

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char msg[] = "roundtrip";
            size_t w = 0;
            STD_INSIST(exec->io()->pwrite(fd, &w, msg, sizeof(msg), 0) == 0);
            STD_INSIST(w == sizeof(msg));

            char buf[32] = {};
            size_t r = 0;
            STD_INSIST(exec->io()->pread(fd, &r, buf, sizeof(buf), 0) == 0);
            STD_INSIST(r == sizeof(msg));
            STD_INSIST(memcmp(buf, msg, sizeof(msg)) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(PreadOffset) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 2);

        exec->spawn([&] {
            int fd = makeTmpFd();

            const char data[] = "0123456789";
            ::write(fd, data, sizeof(data));

            char buf[4] = {};
            size_t n = 0;
            STD_INSIST(exec->io()->pread(fd, &n, buf, 4, 3) == 0);

            STD_INSIST(n == 4);
            STD_INSIST(memcmp(buf, "3456", 4) == 0);

            ::close(fd);
        });

        exec->join();
    }

    STD_TEST(MultipleConcurrent) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        exec->spawn([&] {
            int fd = makeTmpFd();
            const char data[] = "concurrent";
            ::write(fd, data, sizeof(data));

            Semaphore sem(0, exec);

            for (int i = 0; i < 8; ++i) {
                exec->spawn([&] {
                    char buf[16] = {};
                    size_t n = 0;
                    STD_INSIST(exec->io()->pread(fd, &n, buf, sizeof(buf), 0) == 0);
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
