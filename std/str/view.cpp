#include "view.h"
#include "hash.h"

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

u32 StringView::hash32() const noexcept {
    return shash32(data(), length());
}

u64 StringView::hash64() const noexcept {
    return shash64(data(), length());
}

int Std::spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept {
    const auto rr = memCmp(l, r, ll < rl ? ll : rl);

    return rr ? rr : (ll < rl ? -1 : (ll == rl ? 0 : 1));
}

template <>
void Std::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, StringView str) {
    out.write(str.data(), str.length());
}
