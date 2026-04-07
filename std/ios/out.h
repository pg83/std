#pragma once

#include <std/sys/types.h>

namespace stl {
    class FD;
    class Input;
    class Output;
    class ObjPool;
    class ZeroCopyOutput;

    struct CoroExecutor;

    Output* createFDPipe(ObjPool* pool, FD& fd);
    Output* createFDOutput(ObjPool* pool, FD& fd);
    Output* createFDRegular(ObjPool* pool, FD& fd);
    Output* createFDCharacter(ObjPool* pool, FD& fd);
    Output* createCoroFDOutput(ObjPool* pool, FD& fd, CoroExecutor* exec);

    ZeroCopyOutput* createOutBuf(ObjPool* pool, Output& out);
    ZeroCopyOutput* createMemoryOutput(ObjPool* pool, void* ptr);
    ZeroCopyOutput* createOutBuf(ObjPool* pool, Output& out, size_t chunkSize);
}
