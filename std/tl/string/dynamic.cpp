#include "dynamic.h"

#include <std/io/output.h>

using namespace Std;

const char* DynString::cStr() {
    grow(length() + 1);

    *mutEnd() = 0;

    return (const char*)begin();
}

template <>
void Std::output<DynString>(Output& out, const DynString& s) {
    out.write(s.data(), s.length());
}
