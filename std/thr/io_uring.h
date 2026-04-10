#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct IoReactor;

    IoReactor* createIoUringReactor(ObjPool* pool, size_t threads);
}
