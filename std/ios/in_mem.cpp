#include "in_mem.h"
#include "output.h"

using namespace stl;

size_t MemoryInput::nextImpl(const void** chunk) {
    return (*chunk = b, e - b);
}

void MemoryInput::commitImpl(size_t len) {
    b += len;
}

void MemoryInput::sendTo(Output& out) {
    b += out.write(b, e - b);
}
