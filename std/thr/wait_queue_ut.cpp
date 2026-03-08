#include "wait_queue.h"
#include "thread.h"
#include "barrier.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>

using namespace stl;

STD_TEST_SUITE(WaitQueue) {
    STD_TEST(DequeueEmpty) {
        auto wq = WaitQueue::construct(4);
        STD_INSIST(wq->dequeue() == nullptr);
    }

    STD_TEST(EnqueueDequeue) {
        auto wq = WaitQueue::construct(4);

        WaitQueue::Item item;
        item.index = 0;

        wq->enqueue(&item);
        STD_INSIST(wq->dequeue() == &item);
        STD_INSIST(wq->dequeue() == nullptr);
    }

    STD_TEST(MultipleItems) {
        auto wq = WaitQueue::construct(4);

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
        auto wq = WaitQueue::construct(65);

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
        auto wq = WaitQueue::construct(N);
        Barrier ready(N);

        WaitQueue::Item items[N];
        for (int i = 0; i < N; ++i) {
            items[i].index = (u8)i;
        }

        auto worker = [&](int i) {
            return [&, i] {
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
        auto wq = WaitQueue::construct(65);
        Barrier ready(N);
        int enqueued = 0;

        WaitQueue::Item items[N];
        for (int i = 0; i < N; ++i) {
            items[i].index = (u8)i;
        }

        auto worker = [&](int i) {
            return [&, i] {
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
        auto wq = WaitQueue::construct(65);

        WaitQueue::Item items[4];
        for (int i = 0; i < 4; ++i) {
            items[i].index = (u8)i;
            wq->enqueue(&items[i]);
        }

        int found = 0;
        while (auto* item = wq->dequeue()) {
            (void)item;
            ++found;
        }

        STD_INSIST(found == 4);
    }
}
