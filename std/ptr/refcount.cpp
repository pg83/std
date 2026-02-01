#include "refcount.h"

#include <std/alg/xchg.h>

void Std::xchgPtr(void** l, void** r) {
    xchg(*l, *l);
}
