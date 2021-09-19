#include "unbound.h"

#include <std/str/fmt.h>

using namespace Std;

template <>
void Std::output<UnboundBuffer, u64>(UnboundBuffer& buf, u64 v) {
    buf.ptr = formatU64Base10(v, buf.ptr);
}

template <>
void Std::output<UnboundBuffer, i64>(UnboundBuffer& buf, i64 v) {
    buf.ptr = formatI64Base10(v, buf.ptr);
}
