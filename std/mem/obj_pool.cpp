#include "obj_pool.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/exchange.h>

#include <sys/mman.h>

#ifndef MAP_HUGE_2MB
    #define MAP_HUGE_2MB (21 << 26)
#endif

using namespace stl;

namespace {
    constexpr size_t alignment = alignof(max_align_t);
    constexpr size_t HUGE_PAGE_SIZE = (size_t)2 << 20;

    struct alignas(max_align_t) Base: public ObjPool {
        MemoryPool mp;
        Disposer ds;

        Base(void* buf, size_t len)
            : mp(buf, len)
        {
        }
    };

    struct Pool: public Base {
        alignas(max_align_t) u8 buf[256 - sizeof(Base)];

        Pool() noexcept
            : Base(buf, sizeof(buf))
        {
        }

        void* allocate(size_t len) override {
            return mp.allocate(len);
        }

        void submit(Disposable* d) noexcept override {
            ds.submit(d);
        }
    };

    static_assert(sizeof(Pool) == 256);

    // Owns one hugetlb mmap region. Allocated via slave->make<Chunk>:
    // the slave's Disposable chain runs ~Chunk in LIFO order at slave-
    // destruction time, doing the munmap.
    //
    // Throws ChunkMapFailed if the kernel won't satisfy MAP_HUGETLB —
    // fromHugePages catches it on the first chunk to detect "no
    // hugepages support" and fall back. Subsequent failures during
    // operation propagate to the caller as a hard error.
    struct ChunkMapFailed {
    };

    struct Chunk {
        void* page;
        size_t len;

        Chunk(size_t requestedLen)
            : len((requestedLen + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1))
        {
            page = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB,
                        -1, 0);

            if (page == MAP_FAILED) {
                throw ChunkMapFailed{};
            }
        }

        ~Chunk() noexcept {
            munmap(page, len);
        }
    };

    // Bump-allocator over a chain of 2 MiB-hugetlb mmaps, layered on a
    // slave ObjPool*. The slave provides storage for the HugePool
    // object itself, the Chunk descriptors, and any user disposables;
    // the hugetlb mmaps are the actual user-allocation arena.
    //
    // Lifetime: HugePool is allocated via slave->make<HugePool>, so the
    // slave's Disposable chain owns it. The user gets a raw ObjPool*;
    // they keep the slave's Ref alive for as long as they use HugePool.
    //
    // Disposable ordering: every disposable — chunks AND user objects
    // submitted via HugePool::submit — lives in the slave's Disposable
    // chain. submit() forwards straight to slave so the chain
    // interleaves chunks with the user objects that allocated against
    // them. LIFO drain at slave-destruction destroys each user object
    // while its backing chunk is still mapped, then the chunk munmaps,
    // walking back through the chain in submission-reverse order.
    struct alignas(max_align_t) HugePool: public ObjPool {
        ObjPool* slave;
        Chunk* lastChunk;
        u8* cur;
        u8* end;

        HugePool(ObjPool* s, Chunk* first) noexcept;

        void* allocate(size_t len) override;
        void submit(Disposable* d) noexcept override;

    private:
        void addChunk(size_t minLen);
    };

    HugePool::HugePool(ObjPool* s, Chunk* first) noexcept
        : slave(s)
        , lastChunk(first)
        , cur((u8*)first->page)
        , end((u8*)first->page + first->len)
    {
    }

    void* HugePool::allocate(size_t len) {
        const size_t alignedLen = (len + alignment - 1) & ~(alignment - 1);

        if (cur + alignedLen > end) {
            addChunk(alignedLen);
        }

        return exchange(cur, cur + alignedLen);
    }

    void HugePool::submit(Disposable* d) noexcept {
        // Forward to the slave's chain. See class comment for ordering.
        slave->submit(d);
    }

    void HugePool::addChunk(size_t minLen) {
        size_t requested = lastChunk->len * 2;

        while (requested < minLen) {
            requested *= 2;
        }

        auto* c = slave->make<Chunk>(requested);

        lastChunk = c;
        cur = (u8*)c->page;
        end = cur + c->len;
    }
}

ObjPool::~ObjPool() noexcept {
}

void* ObjPool::allocateOverAligned(size_t len, size_t align) {
    auto raw = (uintptr_t)allocate(len + align);

    return (void*)((raw + align - 1) & ~(align - 1));
}

ObjPool* ObjPool::create(ObjPool* pool) {
    return pool->make<Pool>();
}

ObjPool* ObjPool::fromMemoryRaw() {
    return new Pool();
}

ObjPool* ObjPool::fromHugePages(ObjPool* slave) {
    Chunk* first;

    try {
        first = slave->make<Chunk>(HUGE_PAGE_SIZE);
    } catch (const ChunkMapFailed&) {
        return slave;
    }

    return slave->make<HugePool>(slave, first);
}

StringView ObjPool::intern(StringView s) {
    auto len = s.length();
    auto res = (u8*)allocate(len + 1);

    *(u8*)memCpy(res, s.data(), len) = 0;

    return StringView(res, len);
}
