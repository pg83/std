#include "view.h"

#include <std/ios/output.h>

using namespace Std;

template <>
void Std::output<StringView>(Output& out, const StringView& str) {
    out.write(str.data(), str.length());
}
