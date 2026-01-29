#include "char.h"

#include <std/ios/buf.h>

using namespace Std;

static_assert(stdHasTrivialDestructor(OneCharString));
static_assert(sizeof(OneCharString) == 1);

template <>
void Std::output<ZeroCopyOutput, OneCharString>(ZeroCopyOutput& out, OneCharString str) {
    out.write(str.data(), str.length());
}
