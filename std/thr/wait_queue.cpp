#include "wait_queue.h"
#include "mutex.h"

#include <std/sys/types.h>
#include <std/sys/atomic.h>

#include <assert.h>

using namespace stl;

#define STL_WAIT_QUEUE_BITMASK

#ifdef STL_WAIT_QUEUE_BITMASK
struct WaitQueue::Impl {
    u64 bits_ = 0;
    Item* items_[64] = {};

    inline void enqueue(Item* item) noexcept {
        u8 idx = item->index();
        assert(idx < 64);

        items_[idx] = item;
        __atomic_fetch_or(&bits_, u64(1) << idx, __ATOMIC_RELEASE);
    }

    inline Item* dequeue() noexcept {
        u64 old = stdAtomicFetch(&bits_, MemoryOrder::Acquire);

        for (;;) {
            if (!old) {
                return nullptr;
            }

            int idx = __builtin_ctzll(old);
            u64 desired = old & ~(u64(1) << idx);

            if (stdAtomicCAS(&bits_, &old, desired, MemoryOrder::Acquire, MemoryOrder::Acquire)) {
                return items_[idx];
            }
        }
    }
};
#elif __SIZEOF_POINTER__ == 8
struct WaitQueue::Impl {
    // upper 16 bits: tag, lower 48 bits: pointer
    u64 head_ = 0;

    static inline u64 pack(Item* ptr, u64 tag) noexcept {
        return ((tag & 0xFFFFu) << 48) | ((uintptr_t)(ptr) & 0x0000FFFFFFFFFFFFu);
    }

    static inline Item* unpackPtr(u64 val) noexcept {
        return (Item*)(val & 0x0000FFFFFFFFFFFFu);
    }

    static inline u64 unpackTag(u64 val) noexcept {
        return val >> 48;
    }

    inline void enqueue(Item* item) noexcept {
        u64 old = stdAtomicFetch(&head_, MemoryOrder::Relaxed);
        u64 desired;

        do {
            item->next = unpackPtr(old);
            desired = pack(item, unpackTag(old) + 1);
        } while (!stdAtomicCAS(&head_, &old, desired, MemoryOrder::Release, MemoryOrder::Relaxed));
    }

    inline Item* dequeue() noexcept {
        u64 old = stdAtomicFetch(&head_, MemoryOrder::Acquire);

        for (;;) {
            Item* ptr = unpackPtr(old);

            if (!ptr) {
                return nullptr;
            }

            u64 desired = pack(ptr->next, unpackTag(old));

            if (stdAtomicCAS(&head_, &old, desired, MemoryOrder::Acquire, MemoryOrder::Acquire)) {
                return ptr;
            }
        }
    }
};
#else /* __SIZEOF_POINTER__ != 8 */
struct WaitQueue::Impl {
    Mutex mutex;
    Item* head = nullptr;
    int empty = true;

    inline void enqueue(Item* item) noexcept {
        LockGuard lock(mutex);

        item->next = head;
        head = item;

        stdAtomicStore(&empty, false, MemoryOrder::Release);
    }

    inline Item* dequeue() noexcept {
        if (stdAtomicFetch(&empty, MemoryOrder::Acquire)) {
            return nullptr;
        }

        LockGuard lock(mutex);

        Item* item = head;

        if (!item) {
            return nullptr;
        }

        head = item->next;

        stdAtomicStore(&empty, head == nullptr, MemoryOrder::Release);

        return item;
    }
};
#endif /* STL_WAIT_QUEUE_BITMASK / pointer size */

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
