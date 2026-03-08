#pragma once

#include <std/sys/types.h>

namespace stl {
    struct Newable {
        static void* operator new(size_t, void* ptr) {
            return ptr;
        }
    };
}
