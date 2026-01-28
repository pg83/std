#pragma once

#include <std/sys/types.h>

namespace Std {
    class Disposer;

    class MemoryPool {
        u8* currentChunk;
        u8* currentChunkEnd;
        Disposer* ds;

        void allocateNewChunk(size_t minSize);

    public:
        MemoryPool();
        ~MemoryPool() noexcept;

        void* allocate(size_t len);
    };
}
