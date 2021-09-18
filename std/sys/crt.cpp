#include "crt.h"

#include <std/ios/output.h>

#include <stdlib.h>
#include <string.h>

void* Std::allocateMemory(size_t len) {
    if (auto ret = malloc(len); ret) {
        return ret;
    }

    abort();
}

void Std::freeMemory(void* ptr) noexcept {
    free(ptr);
}

int Std::memCmp(const void* l, const void* r, size_t len) noexcept {
    return memcmp(l, r, len);
}
