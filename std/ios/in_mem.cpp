#include "in_mem.h"

#include <std/sys/crt.h>
#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace Std;

size_t MemoryInput::readImpl(void* data, size_t len) {
    const size_t rlen = min<size_t>(len, e - b);

    return (memCpy(data, exchange(b, b + rlen), rlen), rlen);
}
