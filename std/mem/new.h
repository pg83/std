#pragma once

#include <std/sys/types.h>

namespace Std {
    struct Newable {
        static void* operator new(size_t, void* ptr) noexcept {
            return ptr;
        }
    };
}
