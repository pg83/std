#pragma once

#include <std/sys/types.h>

namespace stl {
    class FD;
    class Input;
    class ObjPool;
    class ZeroCopyInput;

    struct CoroExecutor;

    Input* createFDInput(ObjPool* pool, FD& fd);
    Input* createCoroFDInput(ObjPool* pool, FD& fd, CoroExecutor* exec);

    ZeroCopyInput* createInBuf(ObjPool* pool, Input& in);
    ZeroCopyInput* createInBuf(ObjPool* pool, Input& in, size_t chunkSize);
    ZeroCopyInput* createMemoryInput(ObjPool* pool, const void* data, size_t len);
}
