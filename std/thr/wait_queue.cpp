#include "wait_queue.h"
#include "mutex.h"

#include <std/sys/types.h>
#include <std/dbg/assert.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

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

        size_t sleeping() const noexcept override {
            T val = stdAtomicFetch(&bits_, MemoryOrder::Acquire);

            if constexpr (sizeof(T) == 8) {
                return (size_t)__builtin_popcountll(val);
            } else {
                return (size_t)__builtin_popcount(val);
            }
        }
    };

#if __SIZEOF_POINTER__ == 8
    // No ABA here: each Item is a Worker that re-enqueues itself only after being
    // dequeued and completing a full condvar sleep/wake cycle. The tag would need
    // to wrap 65536 times while a competing CAS spins — impossible in practice.
    struct alignas(64) PointerImpl: public WaitQueue {
        // upper 16 bits: tag, lower 48 bits: pointer
        u64 head_ = 0;
        size_t count_ = 0;

        static u64 pack(Item* ptr, u64 tag) noexcept {
            return ((tag & 0xFFFFu) << 48) | ((uintptr_t)(ptr) & 0x0000FFFFFFFFFFFFu);
        }

        static Item* unpackPtr(u64 val) noexcept {
            return (Item*)(val & 0x0000FFFFFFFFFFFFu);
        }

        static u64 unpackTag(u64 val) noexcept {
            return val >> 48;
        }

        void enqueue(Item* item) noexcept override {
            u64 old = stdAtomicFetch(&head_, MemoryOrder::Relaxed);
            u64 desired;

            do {
                item->next = unpackPtr(old);
                desired = pack(item, unpackTag(old) + 1);
            } while (!stdAtomicCAS(&head_, &old, desired, MemoryOrder::Release, MemoryOrder::Relaxed));

            stdAtomicAddAndFetch(&count_, 1, MemoryOrder::Release);
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
                    stdAtomicSubAndFetch(&count_, 1, MemoryOrder::Release);

                    return ptr;
                }
            }
        }

        size_t sleeping() const noexcept override {
            return stdAtomicFetch(&count_, MemoryOrder::Acquire);
        }
    };
#endif

    struct MutexImpl: public WaitQueue {
        Mutex mutex;
        Item* head = nullptr;
        size_t count_ = 0;

        void enqueue(Item* item) noexcept override {
            LockGuard lock(mutex);

            item->next = head;
            head = item;

            stdAtomicAddAndFetch(&count_, 1, MemoryOrder::Release);
        }

        Item* dequeue() noexcept override {
            if (!stdAtomicFetch(&count_, MemoryOrder::Acquire)) {
                return nullptr;
            }

            LockGuard lock(mutex);

            Item* item = head;

            if (!item) {
                return nullptr;
            }

            head = item->next;

            stdAtomicSubAndFetch(&count_, 1, MemoryOrder::Release);

            return item;
        }

        size_t sleeping() const noexcept override {
            return stdAtomicFetch(&count_, MemoryOrder::Acquire);
        }
    };
}

WaitQueue::~WaitQueue() noexcept {
}

WaitQueue* WaitQueue::construct(ObjPool* pool, size_t maxWaiters) {
#if __SIZEOF_POINTER__ == 8
    if (maxWaiters <= 64) {
        return pool->make<BitmaskImpl<u64>>();
    }

    return pool->make<PointerImpl>();
#else
    if (maxWaiters <= 32) {
        return pool->make<BitmaskImpl<u32>>();
    }

    return pool->make<MutexImpl>();
#endif
}
