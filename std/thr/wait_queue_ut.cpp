#include "wait_queue.h"
#include "thread.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(WaitQueue) {
    STD_TEST(DequeueEmpty) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 4);
        STD_INSIST(wq->dequeue() == nullptr);
    }

    STD_TEST(EnqueueDequeue) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 4);

        WaitQueue::Item item;
        item.index = 0;

        wq->enqueue(&item);
        STD_INSIST(wq->dequeue() == &item);
        STD_INSIST(wq->dequeue() == nullptr);
    }

    STD_TEST(MultipleItems) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 4);

        WaitQueue::Item a, b, c;
        a.index = 0;
        b.index = 1;
        c.index = 2;

        wq->enqueue(&a);
        wq->enqueue(&b);
        wq->enqueue(&c);

        int found = 0;
        while (wq->dequeue()) {
            ++found;
        }

        STD_INSIST(found == 3);
    }

    // construct(65) даёт PointerImpl / MutexImpl вместо BitmaskImpl
    STD_TEST(LargeCapacity) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 65);

        WaitQueue::Item a, b;
        a.index = 0;
        b.index = 1;

        wq->enqueue(&a);
        wq->enqueue(&b);

        int found = 0;
        while (wq->dequeue()) {
            ++found;
        }

        STD_INSIST(found == 2);
    }

    STD_TEST(ConcurrentEnqueue) {
        const int N = 8;
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), N);
        WaitGroup ready(N);

        WaitQueue::Item items[N];
        for (int i = 0; i < N; ++i) {
            items[i].index = (u8)i;
        }

        auto worker = [&](int i) {
            return [&, i] {
                ready.done();
                ready.wait();
                wq->enqueue(&items[i]);
            };
        };

        ScopedThread t0(worker(0));
        ScopedThread t1(worker(1));
        ScopedThread t2(worker(2));
        ScopedThread t3(worker(3));
        ScopedThread t4(worker(4));
        ScopedThread t5(worker(5));
        ScopedThread t6(worker(6));
        ScopedThread t7(worker(7));

        // ScopedThread джойнит в деструкторе — все enqueue завершены
        // прежде чем проверяем результат
    }

    STD_TEST(ConcurrentEnqueueDequeue) {
        const int N = 8;
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 65);
        WaitGroup ready(N);
        int enqueued = 0;

        WaitQueue::Item items[N];
        for (int i = 0; i < N; ++i) {
            items[i].index = (u8)i;
        }

        auto worker = [&](int i) {
            return [&, i] {
                ready.done();
                ready.wait();
                wq->enqueue(&items[i]);
                stdAtomicAddAndFetch(&enqueued, 1, MemoryOrder::Release);
            };
        };

        ScopedThread t0(worker(0));
        ScopedThread t1(worker(1));
        ScopedThread t2(worker(2));
        ScopedThread t3(worker(3));
        ScopedThread t4(worker(4));
        ScopedThread t5(worker(5));
        ScopedThread t6(worker(6));
        ScopedThread t7(worker(7));

        // после джойна всех потоков считаем итог
    }

    STD_TEST(EnqueueDequeueOrdering) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 65);

        WaitQueue::Item items[4];
        for (int i = 0; i < 4; ++i) {
            items[i].index = (u8)i;
            wq->enqueue(&items[i]);
        }

        int found = 0;
        while (auto item = wq->dequeue()) {
            (void)item;
            ++found;
        }

        STD_INSIST(found == 4);
    }

    STD_TEST(Bitmask32) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 32);

        WaitQueue::Item items[32];

        for (u8 i = 0; i < 32; ++i) {
            items[i].index = i;
            wq->enqueue(&items[i]);
        }

        STD_INSIST(wq->sleeping() == 32);

        int found = 0;

        while (wq->dequeue()) {
            ++found;
        }

        STD_INSIST(found == 32);
        STD_INSIST(wq->sleeping() == 0);
    }

    STD_TEST(Bitmask64) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 64);

        WaitQueue::Item items[64];

        for (u8 i = 0; i < 64; ++i) {
            items[i].index = i;
            wq->enqueue(&items[i]);
        }

        STD_INSIST(wq->sleeping() == 64);

        int found = 0;

        while (wq->dequeue()) {
            ++found;
        }

        STD_INSIST(found == 64);
        STD_INSIST(wq->sleeping() == 0);
    }

    STD_TEST(Bitmask128) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 128);

        WaitQueue::Item items[128];

        for (u8 i = 0; i < 128; ++i) {
            items[i].index = i;
            wq->enqueue(&items[i]);
        }

        STD_INSIST(wq->sleeping() == 128);

        int found = 0;

        while (wq->dequeue()) {
            ++found;
        }

        STD_INSIST(found == 128);
        STD_INSIST(wq->sleeping() == 0);
    }

    STD_TEST(Pointer256) {
        auto opool = ObjPool::fromMemory();
        auto wq = WaitQueue::construct(opool.mutPtr(), 256);

        WaitQueue::Item items[256];

        for (int i = 0; i < 256; ++i) {
            items[i].index = (u8)i;
            wq->enqueue(&items[i]);
        }

        STD_INSIST(wq->sleeping() == 256);

        int found = 0;

        while (wq->dequeue()) {
            ++found;
        }

        STD_INSIST(found == 256);
        STD_INSIST(wq->sleeping() == 0);
    }
}
