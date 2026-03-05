#include "wait_queue.h"
#include "mutex.h"

#include <std/lib/list.h>
#include <std/sys/atomic.h>

using namespace Std;

struct WaitQueue::Impl {
    Mutex mutex;
    IntrusiveList lst;
    int empty = true;

    inline void enqueue(Item* item) noexcept {
        LockGuard lock(mutex);

        lst.pushBack(item);

        stdAtomicStore(&empty, (int)lst.empty(), MemoryOrder::Release);
    }

    inline Item* dequeue() noexcept {
        if (stdAtomicFetch(&empty, MemoryOrder::Acquire)) {
            return nullptr;
        }

        LockGuard lock(mutex);

        auto res = (Item*)lst.popBackOrNull();

        stdAtomicStore(&empty, (int)lst.empty(), MemoryOrder::Release);

        return res;
    }

    void unlink(Item* item) noexcept {
        LockGuard lock(mutex);

        item->unlink();

        stdAtomicStore(&empty, (int)lst.empty(), MemoryOrder::Release);
    }

    void notifyOne() noexcept {
        if (auto item = dequeue()) {
            item->notify();
        }
    }
};

WaitQueue::WaitQueue()
    : impl(new Impl())
{
}

WaitQueue::~WaitQueue() noexcept {
    delete impl;
}

void WaitQueue::enqueue(Item* item) noexcept {
    impl->enqueue(item);
}

WaitQueue::Item* WaitQueue::dequeue() noexcept {
    return impl->dequeue();
}

void WaitQueue::unlink(Item* item) noexcept {
    impl->unlink(item);
}

void WaitQueue::notifyOne() noexcept {
    impl->notifyOne();
}
