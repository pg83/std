#include "in_mem.h"
#include "output.h"

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
