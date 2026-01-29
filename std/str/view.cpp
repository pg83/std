#include "view.h"

#include <std/ios/buf.h>
#include <std/sys/crt.h>

using namespace Std;

static_assert(stdHasTrivialDestructor(StringView));
static_assert(sizeof(StringView) == 2 * sizeof(void*));

StringView::StringView(const char* s) noexcept
    : StringView((const u8*)s, strLen((const u8*)s))
{
}

template <>
void Std::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, const StringView& str) {
    out.write(str.data(), str.length());
}
