#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/fmt.h>
#include <std/str/view.h>

using namespace Std;

template <>
void Std::output<UnboundBuffer, u64>(UnboundBuffer& buf, u64 v) {
    buf.ptr = formatU64Base10(v, buf.ptr);
}

template <>
void Std::output<UnboundBuffer, i64>(UnboundBuffer& buf, i64 v) {
    buf.ptr = formatI64Base10(v, buf.ptr);
}

template <>
void Std::output<UnboundBuffer, StringView>(UnboundBuffer& buf, const StringView& v) {
    buf.ptr = memCpy(buf.ptr, v.data(), v.length());
}
