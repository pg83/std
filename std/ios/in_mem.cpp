#include "in_mem.h"

#include <std/sys/crt.h>
#include <std/alg/minmax.h>

using namespace Std;

size_t MemoryInput::readImpl(void* data, size_t len) {
    const size_t rlen = min<size_t>(len, e - b);
    memCpy(data, b, rlen);
    return (b += rlen, rlen);
}
