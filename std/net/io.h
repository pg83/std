#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    class ZeroCopyInput;

    ZeroCopyInput* createLimited(ObjPool* pool, ZeroCopyInput* inner, size_t limit);
    ZeroCopyInput* createChunked(ObjPool* pool, ZeroCopyInput* inner);
}
