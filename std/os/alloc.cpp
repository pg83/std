#include "alloc.h"

#include <std/io/output.h>

#include <stdlib.h>

void* Std::allocateMemory(size_t len) {
    if (auto ret = malloc(len); ret) {
        return ret;
    }

    abort();
}

void Std::freeMemory(void* ptr) noexcept {
    free(ptr);
}
