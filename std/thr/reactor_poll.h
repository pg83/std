#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct PollFD;
    struct ThreadPool;
    struct CoroExecutor;

    struct ReactorIface {
        virtual size_t poll(const PollFD* in, PollFD* out, size_t count, u64 deadlineUs) = 0;

        static ReactorIface* create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool);
    };
}
