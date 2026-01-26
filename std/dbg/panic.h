#pragma once

#include <std/sys/types.h>

namespace Std {
    using PanicHandler = void (*)();
    void setPanicHandler(PanicHandler hndl) noexcept;
    void panic(const u8* what, u32 line, const u8* file);
}
