#pragma once

#include <std/sys/types.h>

namespace Std {
    void panic(const c8* what, int line, const c8* file, const c8* func);
}
