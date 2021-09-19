#include "dynamic.h"

#include <std/ios/buf.h>

using namespace Std;

static_assert(sizeof(DynString) == sizeof(void*));

char* DynString::cStr() {
    grow(length() + 1);

    *mutEnd() = 0;

    return (char*)mutBegin();
}

template <>
void Std::output<ZeroCopyOutput, DynString>(ZeroCopyOutput& out, const DynString& s) {
    out.write(s.data(), s.length());
}
