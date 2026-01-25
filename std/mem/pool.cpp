#include "pool.h"

#include <std/sys/crt.h>

#include <std/str/view.h>

#include <std/alg/range.h>
#include <std/alg/reverse.h>

#include <std/lib/vector.h>

#include <cmath>
#include <cstddef>

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }

    struct MemoryPool {
        inline MemoryPool() {
            allocateNewChunk(0);
        }

        inline ~MemoryPool() noexcept {
            for (void* chunk : chunks) {
                freeMemory(chunk);
            }
        }

        inline void* allocate(size_t len) {
            constexpr size_t alignment = alignof(std::max_align_t);
            const size_t alignedLen = (len + alignment - 1) & ~(alignment - 1);

            if (currentChunk + alignedLen > currentChunkEnd) {
                allocateNewChunk(alignedLen);
            }

            void* ptr = currentChunk;
            currentChunk += alignedLen;

            return ptr;
        }

        inline void allocateNewChunk(size_t minSize) {
            size_t nextChunkSize = static_cast<size_t>(1024 * std::pow(2.0, chunks.length()));

            if (nextChunkSize < minSize) {
                nextChunkSize = minSize;
            }

            void* newChunk = allocateMemory(nextChunkSize);

            chunks.pushBack(newChunk);
            currentChunk = static_cast<char*>(newChunk);
            currentChunkEnd = currentChunk + nextChunkSize;
        }

        Vector<void*> chunks;
        char* currentChunk;
        char* currentChunkEnd;
    };

    struct ObjectPool: public Pool {
        MemoryPool mem;
        Vector<Dispose*> obj;

        ~ObjectPool() override {
            reverse(mutRange(obj));

            for (auto ptr : mutRange(obj)) {
                destruct(ptr);
            }
        }

        void* allocate(size_t len) override {
            return mem.allocate(len);
        }

        void submit(Dispose* d) noexcept override {
            obj.pushBack(d);
        }
    };
}

Pool::~Pool() {
}

Pool::Ref Pool::fromMemory() {
    return new ObjectPool();
}

StringView Pool::intern(const StringView& s) {
    auto len = s.length();
    auto res = (u8*)allocate(len);

    memCpy(res, s.data(), len);

    return StringView(res, len);
}

Pool::Dispose::~Dispose() {
}
