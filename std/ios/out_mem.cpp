#include "out_mem.h"

#include <std/sys/crt.h>

using namespace Std;

size_t MemoryOutput::writeImpl(const void* _ptr, size_t len) {
    return (ptr = memCpy(ptr, _ptr, len), len);
}

void* MemoryOutput::imbueImpl(size_t len, size_t* avail) {
    return (*avail = len, ptr);
}

void MemoryOutput::bumpImpl(const void* _ptr) noexcept {
    ptr = (void*)_ptr;
}
