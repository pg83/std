#include "wait_queue.h"
#include "mutex.h"

#include <std/sys/types.h>
#include <std/dbg/assert.h>
#include <std/sys/atomic.h>

using namespace stl;

namespace {
    template <typename T>
    struct BitmaskImpl: public WaitQueue {
        static constexpr u8 N = sizeof(T) * 8;

        T bits_ = 0;
        Item* items_[N] = {};

        void enqueue(Item* item) noexcept override {
            u8 idx = item->index;
            STD_ASSERT(idx < N);

            items_[idx] = item;
            __atomic_fetch_or(&bits_, T(1) << idx, __ATOMIC_RELEASE);
        }

        Item* dequeue() noexcept override {
            T old = stdAtomicFetch(&bits_, MemoryOrder::Acquire);

            for (;;) {
                if (!old) {
                    return nullptr;
                }

                int idx;

                if constexpr (sizeof(T) == 8) {
                    idx = __builtin_ctzll(old);
                } else {
                    idx = __builtin_ctz(old);
                }

                T desired = old & ~(T(1) << idx);

                if (stdAtomicCAS(&bits_, &old, desired, MemoryOrder::Acquire, MemoryOrder::Acquire)) {
                    return items_[idx];
                }
            }
        }
    };

#if __SIZEOF_POINTER__ == 8
    struct PointerImpl: public WaitQueue {
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

        void enqueue(Item* item) noexcept override {
            u64 old = stdAtomicFetch(&head_, MemoryOrder::Relaxed);
            u64 desired;

            do {
                item->next = unpackPtr(old);
                desired = pack(item, unpackTag(old) + 1);
            } while (!stdAtomicCAS(&head_, &old, desired, MemoryOrder::Release, MemoryOrder::Relaxed));
        }

        Item* dequeue() noexcept override {
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
#endif

    struct MutexImpl: public WaitQueue {
        Mutex mutex;
        Item* head = nullptr;
        int empty = true;

        void enqueue(Item* item) noexcept override {
            LockGuard lock(mutex);

            item->next = head;
            head = item;

            stdAtomicStore(&empty, false, MemoryOrder::Release);
        }

        Item* dequeue() noexcept override {
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
}

WaitQueue::~WaitQueue() noexcept {
}

WaitQueue::Ref WaitQueue::construct(size_t maxWaiters) {
#if __SIZEOF_POINTER__ == 8
    if (maxWaiters <= 64) {
        return new BitmaskImpl<u64>();
    }

    return new PointerImpl();
#else
    if (maxWaiters <= 32) {
        return new BitmaskImpl<u32>();
    }

    return new MutexImpl();
#endif
}
