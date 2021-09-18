#pragma once

#if defined(NDEBUG)
#define STD_ASSERT(X)
#else
#include "insist.h"

#define STD_ASSERT(X) STD_INSIST(X)
#endif
