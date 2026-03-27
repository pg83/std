#pragma once

#include <std/sys/types.h>

namespace stl {
    class Output;
    class ObjPool;
    class ZeroCopyInput;

    ZeroCopyInput* createLimitedInput(ObjPool* pool, ZeroCopyInput* inner, size_t limit);
    ZeroCopyInput* createChunkedInput(ObjPool* pool, ZeroCopyInput* inner);

    Output* createLimitedOutput(ObjPool* pool, Output* inner, size_t limit);
    Output* createChunkedOutput(ObjPool* pool, Output* inner);
}
