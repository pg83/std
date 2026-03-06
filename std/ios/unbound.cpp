#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/fmt.h>
#include <std/str/view.h>

using namespace stl;

template <>
void stl::output<UnboundBuffer, u64>(UnboundBuffer& buf, u64 v) {
    buf.ptr = formatU64Base10(v, buf.ptr);
}

template <>
void stl::output<UnboundBuffer, i64>(UnboundBuffer& buf, i64 v) {
    buf.ptr = formatI64Base10(v, buf.ptr);
}

template <>
void stl::output<UnboundBuffer, long double>(UnboundBuffer& buf, long double v) {
    buf.ptr = formatLongDouble(v, buf.ptr);
}

template <>
void stl::output<UnboundBuffer, StringView>(UnboundBuffer& buf, StringView v) {
    buf.ptr = memCpy(buf.ptr, v.data(), v.length());
}
