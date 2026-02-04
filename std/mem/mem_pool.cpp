#include "new.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>
#include <std/sys/types.h>
#include <std/dbg/assert.h>

#include <math.h>

using namespace Std;

namespace {
    constexpr size_t initial = 128;
    constexpr size_t alignment = alignof(max_align_t);

    struct alignas(alignment) ChunkDestructor: public Disposable, public Disposer, public Newable {
    };

    struct alignas(alignment) Chunk: public Disposable, public Newable {
        ~Chunk() noexcept override {
            freeMemory(this);
        }
    };

    struct ChunkDisposer: public Chunk, public Disposer {
    };

    static_assert(sizeof(Chunk) % alignment == 0);
    static_assert(sizeof(ChunkDisposer) % alignment == 0);
    static_assert(sizeof(ChunkDestructor) % alignment == 0);
}

MemoryPool::MemoryPool()
    : currentChunk((u8*)allocateMemory(initial))
    , currentChunkEnd(currentChunk + initial)
    , ds(new (allocate(sizeof(ChunkDisposer))) ChunkDisposer())
{
    ds->submit((ChunkDisposer*)ds);
}

MemoryPool::MemoryPool(void* buf, size_t len) noexcept
    : currentChunk((u8*)buf)
    , currentChunkEnd(currentChunk + len)
    , ds(new (allocate(sizeof(ChunkDestructor))) ChunkDestructor())
{
    STD_ASSERT(len >= initial);
    ds->submit((ChunkDestructor*)ds);
}

MemoryPool::~MemoryPool() noexcept {
    ds->dispose();
}

void* MemoryPool::allocate(size_t len) {
    const size_t alignedLen = (len + alignment - 1) & ~(alignment - 1);

    if (currentChunk + alignedLen > currentChunkEnd) {
        allocateNewChunk(alignedLen + sizeof(Chunk));
    }

    void* ptr = currentChunk;

    currentChunk += alignedLen;

    return ptr;
}

void MemoryPool::allocateNewChunk(size_t minSize) {
    size_t nextChunkSize = static_cast<size_t>(initial * pow(2.0, ds->length()));

    while (nextChunkSize < minSize) {
        nextChunkSize *= 2;
    }

    auto newChunk = new (allocateMemory(nextChunkSize)) Chunk();

    ds->submit(newChunk);

    currentChunkEnd = (u8*)newChunk + nextChunkSize;
    currentChunk = (u8*)(newChunk + 1);

    STD_ASSERT(currentChunkEnd - currentChunk >= minSize);
}
