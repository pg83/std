#include "view.h"

#include <std/ios/buf.h>

using namespace Std;

template <>
void Std::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, const StringView& str) {
    out.write(str.data(), str.length());
}
