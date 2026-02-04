#pragma once

#include <std/sys/types.h>

namespace Std {
    template <typename T>
    inline constexpr T* advancePtr(T* ptr, size_t len) noexcept {
        return (T*)(len + (const u8*)ptr);
    }
}
