#pragma once

#include <std/sys/types.h>
#include <std/lib/vector.h>

using namespace Std;

namespace Std {
    class MemoryPool {
        Vector<void*> chunks;
        u8* currentChunk;
        u8* currentChunkEnd;

        void allocateNewChunk(size_t minSize);

    public:
        MemoryPool();
        ~MemoryPool() noexcept;

        void* allocate(size_t len);
    };
}
