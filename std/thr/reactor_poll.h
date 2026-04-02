#pragma once

#include "coro.h"

namespace stl {
    class ObjPool;

    struct ThreadPool;

    struct ReactorIface {
        virtual u32 poll(int fd, u32 flags, u64 deadlineUs) = 0;
        virtual size_t pollMulti(const PollFD* in, PollFD* out, size_t count, u64 deadlineUs) = 0;

        static ReactorIface* create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool);
    };
}
