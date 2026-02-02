#pragma once

#if defined(NDEBUG) && !defined(ENABLE_ASSERT)
    #define STD_ASSERT(X)
#else
    #include "insist.h"

    #define STD_ASSERT(X) STD_INSIST(X)
#endif
