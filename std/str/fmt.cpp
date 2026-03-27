#include "fmt.h"

#include <std/alg/advance.h>
#include <std/alg/reverse.h>

#include <stdio.h>

void* stl::formatU64Base10(u64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;
    u8* e = b;

    do {
        *e++ = u8'0' + v % 10;
        v /= 10;
    } while (v);

    reverse(b, e);

    return e;
}

void* stl::formatI64Base10(i64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;

    if (v < 0) {
        *b++ = u8'-';

        return formatU64Base10(-(u64)v, b);
    }

    return formatU64Base10(v, b);
}

void* stl::formatU64Base16(u64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;
    u8* e = b;

    do {
        u8 d = v & 0xf;

        *e++ = d < 10 ? u8'0' + d : u8'a' + d - 10;
        v >>= 4;
    } while (v);

    reverse(b, e);

    return e;
}

void* stl::formatLongDouble(long double v, void* buf) noexcept {
    return advancePtr(buf, sprintf((char*)buf, "%Lf", v));
}
