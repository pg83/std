#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct Channel {
        virtual void enqueue(void* v) noexcept = 0;
        virtual void enqueue(void** from, size_t len) noexcept = 0;

        virtual bool dequeue(void** out) noexcept = 0;
        virtual size_t dequeue(void** to, size_t len) noexcept = 0;

        virtual bool tryEnqueue(void* v) noexcept = 0;
        virtual bool tryDequeue(void** out) noexcept = 0;

        virtual void close() noexcept = 0;

        static Channel* create(ObjPool* pool);
        static Channel* create(ObjPool* pool, size_t cap);
        static Channel* create(ObjPool* pool, CoroExecutor* exec);
        static Channel* create(ObjPool* pool, CoroExecutor* exec, size_t cap);
    };
}
