#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct IoReactor;
    struct ThreadPool;
    struct CoroExecutor;

    IoReactor* createPollIoReactor(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, size_t reactors, size_t offloadThreads);
}
