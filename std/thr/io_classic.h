#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct IoReactor;
    struct CoroExecutor;

    IoReactor* createPollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors);
}
