#include "mem_pool.h"

#include <std/sys/crt.h>

#include <cmath>
#include <cstddef>

using namespace Std;

MemoryPool::MemoryPool() {
    allocateNewChunk(0);
}

MemoryPool::~MemoryPool() noexcept {
    for (void* chunk : chunks) {
        freeMemory(chunk);
    }
}

void* MemoryPool::allocate(size_t len) {
    constexpr size_t alignment = alignof(std::max_align_t);
    const size_t alignedLen = (len + alignment - 1) & ~(alignment - 1);

    if (currentChunk + alignedLen > currentChunkEnd) {
        allocateNewChunk(alignedLen);
    }

    void* ptr = currentChunk;
    currentChunk += alignedLen;

    return ptr;
}

void MemoryPool::allocateNewChunk(size_t minSize) {
    size_t nextChunkSize = static_cast<size_t>(128 * std::pow(2.0, chunks.length()));

    if (nextChunkSize < minSize) {
        nextChunkSize = minSize;
    }

    chunks.growDelta(1);
    void* newChunk = allocateMemory(nextChunkSize);
    // will not throw
    chunks.pushBack(newChunk);
    currentChunk = (u8*)(newChunk);
    currentChunkEnd = currentChunk + nextChunkSize;
}
