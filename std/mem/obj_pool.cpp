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

    struct ChunkMapFailed {
    };

    struct Chunk {
        void* page;
        size_t len;

        Chunk(size_t requestedLen);

        ~Chunk() noexcept {
            munmap(page, len);
        }
    };

    struct alignas(max_align_t) HugePool: public ObjPool {
        ObjPool* slave;
        Chunk* lastChunk;
        u8* cur;
        u8* end;

        HugePool(ObjPool* s, Chunk* first) noexcept
            : slave(s)
            , lastChunk(first)
            , cur((u8*)first->page)
            , end((u8*)first->page + first->len)
        {
        }

        void* allocate(size_t len) override;
        void submit(Disposable* d) noexcept override;

    private:
        void addChunk(size_t minLen);
    };
}

Chunk::Chunk(size_t requestedLen)
    : len((requestedLen + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1))
{
    page = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB,
                -1, 0);

    if (page == MAP_FAILED) {
        throw ChunkMapFailed{};
    }
}

void* HugePool::allocate(size_t len) {
    const size_t alignedLen = (len + alignment - 1) & ~(alignment - 1);

    if (cur + alignedLen > end) {
        addChunk(alignedLen);
    }

    return exchange(cur, cur + alignedLen);
}

// Forward to slave so chunks and user disposables interleave in one chain — LIFO drain destroys each user object while its chunk is still mapped.
void HugePool::submit(Disposable* d) noexcept {
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
