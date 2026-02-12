#include "out_mem.h"

#include <std/alg/advance.h>

using namespace Std;

void* MemoryOutput::imbueImpl(size_t* len) {
    return (*len = (size_t)-1, ptr);
}

void MemoryOutput::commitImpl(size_t len) noexcept {
    ptr = advancePtr(ptr, len);
}
