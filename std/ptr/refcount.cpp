#include "refcount.h"

#include <std/alg/xchg.h>

void stl::xchgPtr(void** l, void** r) {
    xchg(*l, *r);
}
