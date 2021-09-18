#include "dynamic.h"

#include <std/ios/output.h>

using namespace Std;

char* DynString::cStr() {
    grow(length() + 1);

    *mutEnd() = 0;

    return (char*)mutBegin();
}

template <>
void Std::output<DynString>(Output& out, const DynString& s) {
    out.write(s.data(), s.length());
}
