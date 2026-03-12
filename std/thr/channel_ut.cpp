#include "coro.h"
#include "channel.h"
#include "pool.h"
#include "latch.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(Channel) {
    STD_TEST(Basic) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 1);
        void* result = nullptr;

        exec->spawn([&]() {
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

        exec->spawn([&]() {
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
        exec->spawn([&]() {
            ch.enqueue((void*)1);
            ch.enqueue((void*)2); // blocks here
        });

        // receiver: drains the channel
        exec->spawn([&]() {
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
        exec->spawn([&]() {
            ch.dequeue(&received);
        });

        // sender: sends after receiver is waiting
        exec->spawn([&]() {
            ch.enqueue((void*)99);
        });

        exec->join();
        STD_INSIST(received == (void*)99);
    }

    STD_TEST(Close) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 4);
        bool got = true;

        exec->spawn([&]() {
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
            exec->spawn([&]() {
                void* v;
                bool ok = ch.dequeue(&v);
                if (!ok) {
                    stdAtomicAddAndFetch(&falseCount, 1, MemoryOrder::Relaxed);
                }
            });
        }

        exec->spawn([&]() {
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

        exec->spawn([&]() {
            for (int i = 0; i < N; ++i) {
                ch.enqueue((void*)(uintptr_t)(i + 1));
            }
            ch.close();
        });

        exec->spawn([&]() {
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
        Latch prodDone(nProducers);
        int consumed = 0;

        for (int i = 0; i < nProducers; ++i) {
            exec->spawn([&]() {
                for (int j = 0; j < nPerProducer; ++j) {
                    ch.enqueue((void*)(uintptr_t)(j + 1));
                }
                prodDone.arrive();
            });
        }

        for (int i = 0; i < nConsumers; ++i) {
            exec->spawn([&]() {
                void* v;
                while (ch.dequeue(&v)) {
                    stdAtomicAddAndFetch(&consumed, 1, MemoryOrder::Relaxed);
                }
            });
        }

        prodDone.wait();
        exec->spawn([&]() {
            ch.close();
        });

        exec->join();
        STD_INSIST(consumed == total);
    }

    STD_TEST(TryEnqueueDequeue) {
        auto exec = CoroExecutor::create(4);
        Channel ch(exec.mutPtr(), 2);

        exec->spawn([&]() {
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

    STD_TEST(Stress) {
        auto exec = CoroExecutor::create(8);
        Channel ch(exec.mutPtr(), 16);
        const int nProducers = 8;
        const int nConsumers = 8;
        const int nPerProducer = 200;
        const int total = nProducers * nPerProducer;
        Latch prodDone(nProducers);
        int consumed = 0;

        for (int i = 0; i < nProducers; ++i) {
            exec->spawn([&]() {
                for (int j = 0; j < nPerProducer; ++j) {
                    ch.enqueue((void*)(uintptr_t)(j + 1));
                }
                prodDone.arrive();
            });
        }

        for (int i = 0; i < nConsumers; ++i) {
            exec->spawn([&]() {
                void* v;
                while (ch.dequeue(&v)) {
                    stdAtomicAddAndFetch(&consumed, 1, MemoryOrder::Relaxed);
                }
            });
        }

        prodDone.wait();
        exec->spawn([&]() {
            ch.close();
        });

        exec->join();
        STD_INSIST(consumed == total);
    }
}
