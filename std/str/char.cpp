#include "char.h"

#include <std/ios/buf.h>

using namespace Std;

template <>
void Std::output<OneCharString>(OutBuf& out, const OneCharString& str) {
    out.write(str.data(), str.length());
}
