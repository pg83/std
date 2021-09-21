#include "view.h"

#include <std/ios/buf.h>

using namespace Std;

static_assert(!Traits::HasDestructor<StringView>::R);

template <>
void Std::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, const StringView& str) {
    out.write(str.data(), str.length());
}
