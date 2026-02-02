#pragma once

#include <std/sys/types.h>

namespace Std {
    using PanicHandler = void (*)();

    PanicHandler setPanicHandler1(PanicHandler hndl) noexcept;
    PanicHandler setPanicHandler2(PanicHandler hndl) noexcept;

    void panic(const u8* what, u32 line, const u8* file);
}
