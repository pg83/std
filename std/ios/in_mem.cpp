#include "in_mem.h"

#include <std/sys/crt.h>
#include <std/alg/minmax.h>

using namespace Std;

size_t MemoryInput::readImpl(void* data, size_t len) {
    const size_t rlen = min<size_t>(len, e - b);

    return (b = (const u8*)memCpy(data, b, rlen), rlen);
}
