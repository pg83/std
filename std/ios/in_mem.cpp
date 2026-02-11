#include "in_mem.h"
#include "output.h"

#include <std/sys/crt.h>
#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace Std;

size_t MemoryInput::nextImpl(const void** chunk) {
    return (*chunk = b, e - b);
}

void MemoryInput::commitImpl(size_t len) noexcept {
    b += len;
}

void MemoryInput::sendTo(Output& out) {
    b += out.write(b, e - b);
}
