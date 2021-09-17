#include "char.h"

#include <std/io/output.h>

using namespace Std;

template <>
void Std::output<OneCharString>(Output& out, const OneCharString& str) {
    out.write(str.data(), str.length());
}
