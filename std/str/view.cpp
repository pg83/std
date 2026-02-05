#include "view.h"
#include "hash.h"

#include <std/ios/buf.h>
#include <std/sys/crt.h>
#include <std/lib/buffer.h>

using namespace Std;

static_assert(stdHasTrivialDestructor(StringView));
static_assert(sizeof(StringView) == 2 * sizeof(void*));

namespace {
    inline int spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept {
        const auto rr = memCmp(l, r, ll < rl ? ll : rl);

        return rr ? rr : (ll < rl ? -1 : (ll == rl ? 0 : 1));
    }

    inline int spaceship(const StringView& l, const StringView& r) noexcept {
        return spaceship(l.data(), l.length(), r.data(), r.length());
    }
}

bool Std::operator==(const StringView& l, const StringView& r) noexcept {
    return spaceship(l, r) == 0;
}

bool Std::operator!=(const StringView& l, const StringView& r) noexcept {
    return !(l == r);
}

bool Std::operator<(const StringView& l, const StringView& r) noexcept {
    return spaceship(l, r) < 0;
}

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

StringView StringView::prefix(size_t len) const noexcept {
    if (len > len_) {
        return *this;
    }

    return StringView(ptr_, len);
}

bool StringView::startsWith(StringView prefix) const noexcept {
    if (prefix.length() > len_) {
        return false;
    }

    return this->prefix(prefix.length()) == prefix;
}

template <>
void Std::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, StringView str) {
    out.write(str.data(), str.length());
}
