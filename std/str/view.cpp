#include "view.h"

#include <std/ios/buf.h>
#include <std/sys/crt.h>
#include <std/lib/buffer.h>

using namespace Std;

static_assert(stdHasTrivialDestructor(StringView));
static_assert(sizeof(StringView) == 2 * sizeof(void*));

StringView::StringView(const char* s) noexcept
    : StringView((const u8*)s, strLen((const u8*)s))
{
}

StringView::StringView(const Buffer& b) noexcept
    : StringView((const u8*)b.data(), b.length())
{
}

template <>
void Std::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, const StringView& str) {
    out.write(str.data(), str.length());
}
