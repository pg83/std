#include "coro.h"
#include "pool.h"
#include "thread.h"
#include "channel.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void dutchRudder(int nStages, int nMessages) {
        auto exec = CoroExecutor::create(8);
        auto opool = ObjPool::fromMemory();

        Vector<Channel*> chArr;

        for (int i = 0; i <= nStages; ++i) {
            chArr.pushBack(opool->make<Channel>(exec.mutPtr(), (size_t)5));
        }

        for (int i = 0; i < nStages; ++i) {
            exec->spawn([in = chArr[i], out = chArr[i + 1]] {
                void* v;
                while (in->dequeue(&v)) {
                    out->enqueue(v);
                }
                out->close();
            });
        }

        i64 sum = 0;

        exec->spawn([ch = chArr[nStages], &sum] {
            void* v;
            while (ch->dequeue(&v)) {
                sum += (i64)(uintptr_t)v;
            }
        });

        exec->spawn([ch = chArr[0], nMessages] {
            for (int i = 1; i <= nMessages; ++i) {
                ch->enqueue((void*)(uintptr_t)i);
            }
            ch->close();
        });

        exec->join();
        STD_INSIST(sum == (i64)nMessages * (nMessages + 1) / 2);
    }
}

STD_TEST_SUITE(ChannelThreaded) {
    STD_TEST(Basic) {
        Channel ch(1);
        void* result = nullptr;

        {
            ScopedThread t([&] {
                ch.dequeue(&result);
            });

            ch.enqueue((void*)42);
        }

        STD_INSIST(result == (void*)42);
    }

    STD_TEST(BlockingSender) {
        Channel ch;
        void* v1 = nullptr;
        void* v2 = nullptr;

        {
            ScopedThread receiver([&] {
                ch.dequeue(&v1);
                ch.dequeue(&v2);
            });

            ch.enqueue((void*)1);
            ch.enqueue((void*)2);
        }

        STD_INSIST(v1 == (void*)1);
        STD_INSIST(v2 == (void*)2);
    }

    STD_TEST(ProducerConsumer) {
        Channel ch(4);
        const int N = 100;
        int sum = 0;

        {
            ScopedThread producer([&] {
                for (int i = 1; i <= N; ++i) {
                    ch.enqueue((void*)(uintptr_t)i);
                }
                ch.close();
            });

            ScopedThread consumer([&] {
                void* v;
                while (ch.dequeue(&v)) {
                    sum += (int)(uintptr_t)v;
                }
            });
        }

        STD_INSIST(sum == N * (N + 1) / 2);
    }

    STD_TEST(MPMC) {
        Channel ch(8);
        const int nProducers = 4;
        const int nPerProducer = 50;
        const int total = nProducers * nPerProducer;
        int consumed = 0;
        int prodDone = 0;

        auto produce = [&] {
            for (int j = 0; j < nPerProducer; ++j) {
                ch.enqueue((void*)(uintptr_t)(j + 1));
            }

            if (stdAtomicAddAndFetch(&prodDone, 1, MemoryOrder::Release) == nProducers) {
                ch.close();
            }
        };

        auto consume = [&] {
            void* v;
            while (ch.dequeue(&v)) {
                stdAtomicAddAndFetch(&consumed, 1, MemoryOrder::Relaxed);
            }
        };

        {
            ScopedThread p0(produce), p1(produce), p2(produce), p3(produce);
            ScopedThread c0(consume), c1(consume), c2(consume), c3(consume);
        }

        STD_INSIST(consumed == total);
    }

    STD_TEST(TryEnqueueDequeue) {
        Channel ch(2);
        void* v;

        STD_INSIST(!ch.tryDequeue(&v));

        STD_INSIST(ch.tryEnqueue((void*)1));
        STD_INSIST(ch.tryEnqueue((void*)2));
        STD_INSIST(!ch.tryEnqueue((void*)3));

        STD_INSIST(ch.tryDequeue(&v));
        STD_INSIST(v == (void*)1);

        STD_INSIST(ch.tryDequeue(&v));
        STD_INSIST(v == (void*)2);

        STD_INSIST(!ch.tryDequeue(&v));
    }
}

STD_TEST_SUITE(Channel) {
    STD_TEST(Basic) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 1);
        void* result = nullptr;

        exec->spawn([&] {
            ch.enqueue((void*)42);
            ch.dequeue(&result);
        });

        exec->join();
        STD_INSIST(result == (void*)42);
    }

    STD_TEST(Buffered) {
        auto exec = CoroExecutor::create(4);
        const size_t cap = 8;
        Channel ch(exec.mutPtr(), cap);
        int count = 0;

        exec->spawn([&] {
            for (size_t i = 0; i < cap; ++i) {
                ch.enqueue((void*)(uintptr_t)i);
            }

            for (size_t i = 0; i < cap; ++i) {
                void* v;
                ch.dequeue(&v);
                if ((uintptr_t)v == i) {
                    ++count;
                }
            }
        });

        exec->join();
        STD_INSIST(count == (int)cap);
    }

    STD_TEST(BlockingSender) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 1);
        void* received = nullptr;

        // sender: will block after first enqueue since cap=1
        exec->spawn([&] {
            ch.enqueue((void*)1);
            ch.enqueue((void*)2); // blocks here
        });

        // receiver: drains the channel
        exec->spawn([&] {
            void* v1;
            void* v2;
            ch.dequeue(&v1);
            ch.dequeue(&v2);
            received = v2;
        });

        exec->join();
        STD_INSIST(received == (void*)2);
    }

    STD_TEST(BlockingReceiver) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 1);
        void* received = nullptr;

        // receiver: blocks waiting for value
        exec->spawn([&] {
            ch.dequeue(&received);
        });

        // sender: sends after receiver is waiting
        exec->spawn([&] {
            ch.enqueue((void*)99);
        });

        exec->join();
        STD_INSIST(received == (void*)99);
    }

    STD_TEST(Close) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 4);
        bool got = true;

        exec->spawn([&] {
            ch.enqueue((void*)1);
            ch.close();

            void* v;
            ch.dequeue(&v);       // gets 1
            got = ch.dequeue(&v); // returns false
        });

        exec->join();
        STD_INSIST(!got);
    }

    STD_TEST(CloseWithPendingReceivers) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 1);
        const int N = 4;
        int falseCount = 0;

        for (int i = 0; i < N; ++i) {
            exec->spawn([&] {
                void* v;
                bool ok = ch.dequeue(&v);
                if (!ok) {
                    stdAtomicAddAndFetch(&falseCount, 1, MemoryOrder::Relaxed);
                }
            });
        }

        exec->spawn([&] {
            ch.close();
        });

        exec->join();
        STD_INSIST(falseCount == N);
    }

    STD_TEST(ProducerConsumer) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 4);
        const int N = 100;
        int sum = 0;

        exec->spawn([&] {
            for (int i = 0; i < N; ++i) {
                ch.enqueue((void*)(uintptr_t)(i + 1));
            }
            ch.close();
        });

        exec->spawn([&] {
            void* v;
            while (ch.dequeue(&v)) {
                stdAtomicAddAndFetch(&sum, (int)(uintptr_t)v, MemoryOrder::Relaxed);
            }
        });

        exec->join();
        STD_INSIST(sum == N * (N + 1) / 2);
    }

    STD_TEST(MPMC) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 8);
        const int nProducers = 4;
        const int nConsumers = 4;
        const int nPerProducer = 50;
        const int total = nProducers * nPerProducer;
        WaitGroup prodDone(nProducers);
        int consumed = 0;

        for (int i = 0; i < nProducers; ++i) {
            exec->spawn([&] {
                for (int j = 0; j < nPerProducer; ++j) {
                    ch.enqueue((void*)(uintptr_t)(j + 1));
                }
                prodDone.done();
            });
        }

        for (int i = 0; i < nConsumers; ++i) {
            exec->spawn([&] {
                void* v;
                while (ch.dequeue(&v)) {
                    stdAtomicAddAndFetch(&consumed, 1, MemoryOrder::Relaxed);
                }
            });
        }

        prodDone.wait();
        exec->spawn([&] {
            ch.close();
        });

        exec->join();
        STD_INSIST(consumed == total);
    }

    STD_TEST(TryEnqueueDequeue) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 2);

        exec->spawn([&] {
            void* v;

            STD_INSIST(!ch.tryDequeue(&v));

            STD_INSIST(ch.tryEnqueue((void*)1));
            STD_INSIST(ch.tryEnqueue((void*)2));
            STD_INSIST(!ch.tryEnqueue((void*)3)); // full

            STD_INSIST(ch.tryDequeue(&v));
            STD_INSIST(v == (void*)1);

            STD_INSIST(ch.tryDequeue(&v));
            STD_INSIST(v == (void*)2);

            STD_INSIST(!ch.tryDequeue(&v));
        });

        exec->join();
    }

    STD_TEST(Pipeline) {
        dutchRudder(50, 500);
    }

    STD_TEST(_Pipeline) {
        dutchRudder(2000, 200000);
    }

    STD_TEST(Stress) {
        auto exec = CoroExecutor::create(8);
        Channel ch(exec.mutPtr(), 16);
        const int nProducers = 8;
        const int nConsumers = 8;
        const int nPerProducer = 200;
        const int total = nProducers * nPerProducer;
        WaitGroup prodDone(nProducers);
        int consumed = 0;

        for (int i = 0; i < nProducers; ++i) {
            exec->spawn([&] {
                for (int j = 0; j < nPerProducer; ++j) {
                    ch.enqueue((void*)(uintptr_t)(j + 1));
                }
                prodDone.done();
            });
        }

        for (int i = 0; i < nConsumers; ++i) {
            exec->spawn([&] {
                void* v;
                while (ch.dequeue(&v)) {
                    stdAtomicAddAndFetch(&consumed, 1, MemoryOrder::Relaxed);
                }
            });
        }

        prodDone.wait();
        exec->spawn([&] {
            ch.close();
        });

        exec->join();
        STD_INSIST(consumed == total);
    }
}
