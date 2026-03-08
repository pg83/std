#pragma once

#include <std/sys/types.h>

namespace stl {
    template <typename T>
    inline T* advancePtr(T* ptr, size_t len) noexcept {
        return (T*)(len + (const u8*)ptr);
    }
}
