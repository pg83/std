#include "out_mem.h"

#include <std/sys/crt.h>
#include <std/alg/advance.h>

using namespace Std;

size_t MemoryOutput::writeImpl(const void* _ptr, size_t len) {
    return (ptr = memCpy(ptr, _ptr, len), len);
}

void* MemoryOutput::imbueImpl(size_t* len) {
    return ptr;
}

void MemoryOutput::commitImpl(size_t len) noexcept {
    ptr = advancePtr(ptr, len);
}
