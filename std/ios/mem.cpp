#include "mem.h"

#include <std/sys/crt.h>

using namespace Std;

void MemoryOutput::writeImpl(const void* _ptr, size_t len) {
    ptr = memCpy(ptr, _ptr, len);
}

void* MemoryOutput::imbueImpl(size_t) {
    return ptr;
}

void MemoryOutput::bumpImpl(const void* _ptr) noexcept {
    ptr = (void*)_ptr;
}
