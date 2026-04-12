#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct PollFD;
    struct PollerIface;
    struct CoroExecutor;

    struct ReactorIface {
        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual PollerIface* createPoller(ObjPool* pool) = 0;

        static ReactorIface* create(CoroExecutor* exec, ObjPool* opool);
    };
}
