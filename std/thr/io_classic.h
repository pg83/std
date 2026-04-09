#pragma once

namespace stl {
    class ObjPool;

    struct IoReactor;
    struct ThreadPool;
    struct CoroExecutor;

    IoReactor* createPollIoReactor(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, ThreadPool* offload);
}
