#pragma once

#include "panic.h"

#include <std/sys/types.h>

#define STD_CAT(X, Y) STD_CA_(X, Y)
#define STD_CA_(X, Y) STD_C__(X, Y)
#define STD_C__(X, Y) X##Y

#define STD_STRINGIZE(X) STD_STRINGIZ_(X)
#define STD_STRINGIZ_(X) #X

#define STD_INSIST(X)                      \
    if (!(X)) {                            \
        ::Std::panic(                      \
            STD_CAT(u8, STD_STRINGIZE(X)), \
            __LINE__,                      \
            STD_CAT(u8, __FILE__));        \
    }
