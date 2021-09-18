#pragma once

#if defined(NDEBUG)
#include "insist.h"

#define STD_ASSERT(X) STD_INSIST(X)
#else
#define STD_ASSERT(X)
#endif
