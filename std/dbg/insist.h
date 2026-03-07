#pragma once

#include "panic.h"

#include <std/sys/types.h>

#define STD_CAT(X, Y) STD_CA_(X, Y)
#define STD_CA_(X, Y) STD_C__(X, Y)
#define STD_C__(X, Y) X##Y

#define STD_STR(X) STD_ST_(X)
#define STD_ST_(X) #X

#define STD_INSIST(X)                   \
    do {                                \
        if (!(X)) {                     \
            ::stl::panic(               \
                STD_CAT(u8,             \
                        STD_STR(X)),    \
                __LINE__,               \
                STD_CAT(u8, __FILE__)); \
        }                               \
    } while (false)
