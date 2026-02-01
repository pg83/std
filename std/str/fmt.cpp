#include "fmt.h"

#include <std/lib/buffer.h>
#include <std/alg/reverse.h>

#include <stdio.h>

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

    if (v < 0) {
        *b++ = u8'-';

        return formatU64Base10((u64)(-(v + (i64)1)) + (u64)1, b);
    }

    return formatU64Base10(v, b);
}

void* Std::formatLongDouble(long double v, void* buf) noexcept {
    return advancePtr(buf, sprintf((char*)buf, "%Lf", v));
}
