#pragma once

#include <std/sys/types.h>

namespace stl {
    using PanicHandler = void (*)();

    PanicHandler setPanicHandler1(PanicHandler hndl);
    PanicHandler setPanicHandler2(PanicHandler hndl);

    void panic(const u8* what, u32 line, const u8* file);
}
