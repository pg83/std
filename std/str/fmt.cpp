#include "fmt.h"

#include <std/alg/reverse.h>

void* Std::formatU64Base10(u64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;
    u8* e = b;

    do {
        *e++ = u8'0' + v % 10;
        v /= 10;
    } while (v);

    reverse(b, e);

    return e;
}

void* Std::formatI64Base10(i64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;

    // TODO(pg): incorrent handling of lowest int
    if (v < 0) {
        *b++ = u8'-';

        return formatU64Base10(-v, b);
    }

    return formatU64Base10(v, b);
}
