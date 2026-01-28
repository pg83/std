#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>

#include <new>

#include <cmath>
#include <cstddef>

using namespace Std;

namespace {
    constexpr size_t alignment = alignof(std::max_align_t);

    struct alignas(alignment) Chunk: public Disposable {
        ~Chunk() noexcept override {
            freeMemory(this);
        }
    };

    struct ChunkDisposer: public Chunk, public Disposer {
    };
}

MemoryPool::MemoryPool()
    : currentChunk((u8*)allocateMemory(128))
    , currentChunkEnd(currentChunk + 128)
    , ds(new (allocate(sizeof(ChunkDisposer))) ChunkDisposer())
{
    ds->submit((ChunkDisposer*)ds);
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
    size_t nextChunkSize = static_cast<size_t>(128 * std::pow(2.0, ds->length()));

    if (nextChunkSize < minSize) {
        nextChunkSize = minSize;
    }

    auto newChunk = new (allocateMemory(nextChunkSize)) Chunk();

    ds->submit(newChunk);

    currentChunkEnd = (u8*)newChunk + nextChunkSize;
    currentChunk = (u8*)(newChunk + 1);
}
