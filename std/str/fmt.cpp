#include "fmt.h"

size_t Std::formatU64Base10(u64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;
    u8* e = b;

    do {
        *e++ = '0' + v % 10;
        v /= 10;
    } while (v);

    return e - b;
}
