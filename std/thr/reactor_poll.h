#pragma once

#include "poller.h"

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct ThreadPool;
    struct CoroExecutor;

    struct ReactorIface {
        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual size_t pollMulti(const PollFD* in, PollFD* out, size_t count, u64 deadlineUs) = 0;

        static ReactorIface* create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool);
    };
}
