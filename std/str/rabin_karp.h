#pragma once

#include <std/sys/types.h>

namespace Std {
    bool findRK(const u8* str, size_t strLen, const u8* substr, size_t substrLen, size_t* pos) noexcept;
}
