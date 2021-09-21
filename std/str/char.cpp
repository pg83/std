#include "char.h"

#include <std/ios/buf.h>

using namespace Std;

static_assert(!Traits::HasDestructor<OneCharString>::R);

template <>
void Std::output<ZeroCopyOutput, OneCharString>(ZeroCopyOutput& out, OneCharString str) {
    out.write(str.data(), str.length());
}
