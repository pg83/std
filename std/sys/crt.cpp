#include "crt.h"

#include <std/dbg/insist.h>
#include <std/ios/output.h>

#include <stdlib.h>
#include <string.h>

void* Std::allocateMemory(size_t len) {
    if (auto ret = malloc(len); ret) {
        return ret;
    }

    STD_INSIST(false);

    return 0;
}

void* Std::allocateZeroedMemory(size_t count, size_t el) {
    if (auto ret = calloc(count, el); ret) {
        return ret;
    }

    STD_INSIST(false);

    return 0;
}

void Std::freeMemory(void* ptr) noexcept {
    free(ptr);
}

int Std::memCmp(const void* l, const void* r, size_t len) noexcept {
    if (len == 0) {
        return 0;
    }

    return memcmp(l, r, len);
}

void* Std::memCpy(void* to, const void* from, size_t len) noexcept {
    memcpy(to, from, len);

    return len + (u8*)to;
}

size_t Std::strLen(const u8* s) noexcept {
    return s ? strlen((const char*)s) : 0;
}
