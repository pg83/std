#include "coro.h"
#include "pool.h"
#include "thread.h"
#include "runable.h"
#include "channel.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/alg/defer.h>
#include <std/sys/atomic.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void dutchRudder(int nStages, int nMessages) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 8);

        Vector<Channel*> chArr;

        for (int i = 0; i <= nStages; ++i) {
            chArr.pushBack(Channel::create(opool.mutPtr(), exec, (size_t)5));
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
        auto pool = ObjPool::fromMemory();
        auto& ch = *Channel::create(pool.mutPtr(), (size_t)1);
        void* result = nullptr;

        {
            auto r = makeRunable([&] {
                ch.dequeue(&result);
            });

            auto* t = Thread::create(pool.mutPtr(), r);
            STD_DEFER {
                t->join();
            };

            ch.enqueue((void*)42);
        }

        STD_INSIST(result == (void*)42);
    }

    STD_TEST(BlockingSender) {
        auto pool = ObjPool::fromMemory();
        auto& ch = *Channel::create(pool.mutPtr());
        void* v1 = nullptr;
        void* v2 = nullptr;

        {
            auto r = makeRunable([&] {
                ch.dequeue(&v1);
                ch.dequeue(&v2);
            });

            auto* t = Thread::create(pool.mutPtr(), r);
            STD_DEFER {
                t->join();
            };

            ch.enqueue((void*)1);
            ch.enqueue((void*)2);
        }

        STD_INSIST(v1 == (void*)1);
        STD_INSIST(v2 == (void*)2);
    }

    STD_TEST(ProducerConsumer) {
        auto pool = ObjPool::fromMemory();
        auto& ch = *Channel::create(pool.mutPtr(), (size_t)4);
        const int N = 100;
        int sum = 0;

        {
            auto rp = makeRunable([&] {
                for (int i = 1; i <= N; ++i) {
                    ch.enqueue((void*)(uintptr_t)i);
                }
                ch.close();
            });

            auto rc = makeRunable([&] {
                void* v;
                while (ch.dequeue(&v)) {
                    sum += (int)(uintptr_t)v;
                }
            });

            auto* tp = Thread::create(pool.mutPtr(), rp);
            STD_DEFER {
                tp->join();
            };
            auto* tc = Thread::create(pool.mutPtr(), rc);
            STD_DEFER {
                tc->join();
            };
        }

        STD_INSIST(sum == N * (N + 1) / 2);
    }

    STD_TEST(MPMC) {
        auto pool = ObjPool::fromMemory();
        auto& ch = *Channel::create(pool.mutPtr(), (size_t)8);
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
            auto rp0 = makeRunable(produce);
            auto rp1 = makeRunable(produce);
            auto rp2 = makeRunable(produce);
            auto rp3 = makeRunable(produce);
            auto rc0 = makeRunable(consume);
            auto rc1 = makeRunable(consume);
            auto rc2 = makeRunable(consume);
            auto rc3 = makeRunable(consume);

            auto* p0 = Thread::create(pool.mutPtr(), rp0);
            STD_DEFER {
                p0->join();
            };
            auto* p1 = Thread::create(pool.mutPtr(), rp1);
            STD_DEFER {
                p1->join();
            };
            auto* p2 = Thread::create(pool.mutPtr(), rp2);
            STD_DEFER {
                p2->join();
            };
            auto* p3 = Thread::create(pool.mutPtr(), rp3);
            STD_DEFER {
                p3->join();
            };
            auto* c0 = Thread::create(pool.mutPtr(), rc0);
            STD_DEFER {
                c0->join();
            };
            auto* c1 = Thread::create(pool.mutPtr(), rc1);
            STD_DEFER {
                c1->join();
            };
            auto* c2 = Thread::create(pool.mutPtr(), rc2);
            STD_DEFER {
                c2->join();
            };
            auto* c3 = Thread::create(pool.mutPtr(), rc3);
            STD_DEFER {
                c3->join();
            };
        }

        STD_INSIST(consumed == total);
    }

    STD_TEST(TryEnqueueDequeue) {
        auto pool = ObjPool::fromMemory();
        auto& ch = *Channel::create(pool.mutPtr(), (size_t)2);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)1);
        void* result = nullptr;

        exec->spawn([&] {
            ch.enqueue((void*)42);
            ch.dequeue(&result);
        });

        exec->join();
        STD_INSIST(result == (void*)42);
    }

    STD_TEST(Buffered) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const size_t cap = 8;
        auto& ch = *Channel::create(pool.mutPtr(), exec, cap);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)1);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)1);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)4);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)1);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)4);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)8);
        const int nProducers = 4;
        const int nConsumers = 4;
        const int nPerProducer = 50;
        const int total = nProducers * nPerProducer;
        auto& prodDone = *WaitGroup::create(pool.mutPtr(), nProducers);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)2);

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
        dutchRudder(10, 100);
    }

    STD_TEST(Stress) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 8);
        auto& ch = *Channel::create(pool.mutPtr(), exec, (size_t)16);
        const int nProducers = 4;
        const int nConsumers = 4;
        const int nPerProducer = 100;
        const int total = nProducers * nPerProducer;
        auto& prodDone = *WaitGroup::create(pool.mutPtr(), nProducers);
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
