#pragma once

#include "insist.h"

#include <std/sys/throw.h>

#define STD_VERIFY(X)                      \
    do {                                   \
        if (!(X)) {                        \
            ::stl::raiseVerify(            \
                STD_CAT(u8,                \
                        STD_STR(X)),       \
                __LINE__,                  \
                STD_CAT(u8, __FILE__));    \
        }                                  \
    } while (false)
