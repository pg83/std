#include "view.h"
#include "hash.h"

#include <std/sys/crt.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>
#include <std/ios/out_zc.h>

#define _GNU_SOURCE
#include <string.h>

using namespace stl;

static_assert(stdHasTrivialDestructor(StringView));
static_assert(sizeof(StringView) == 2 * sizeof(void*));

namespace {
    static int spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept {
        const auto rr = memCmp(l, r, ll < rl ? ll : rl);

        return rr ? rr : (ll < rl ? -1 : (ll == rl ? 0 : 1));
    }

    static int spaceship(StringView l, StringView r) noexcept {
        return spaceship(l.data(), l.length(), r.data(), r.length());
    }

    static const u8* fix(const u8* ptr) noexcept {
        return ptr ? ptr : u8"";
    }
}

bool stl::operator==(StringView l, StringView r) noexcept {
    return l.length() == r.length() && spaceship(l, r) == 0;
}

bool stl::operator!=(StringView l, StringView r) noexcept {
    return !(l == r);
}

bool stl::operator<(StringView l, StringView r) noexcept {
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
    return StringView(ptr_, min(len, len_));
}

StringView StringView::suffix(size_t len) const noexcept {
    return StringView(end() - min(len, len_), end());
}

bool StringView::startsWith(StringView prefix) const noexcept {
    return this->prefix(prefix.length()) == prefix;
}

bool StringView::endsWith(StringView suffix) const noexcept {
    return this->suffix(suffix.length()) == suffix;
}

const u8* StringView::search(StringView substr) const noexcept {
    return (const u8*)memmem(fix(data()), length(), fix(substr.data()), substr.length());
}

const u8* StringView::memChr(u8 ch) const noexcept {
    return (const u8*)memchr(fix(data()), ch, length());
}

StringView StringView::stripCr() const noexcept {
    if (!empty() && back() == '\r') {
        return prefix(length() - 1);
    }

    return *this;
}

StringView StringView::stripSpace() const noexcept {
    const u8* p = data();
    const u8* e = end();

    while (p < e && *p == ' ') {
        ++p;
    }

    while (e > p && *(e - 1) == ' ') {
        --e;
    }

    return StringView(p, e);
}

bool StringView::split(u8 delim, StringView& before, StringView& after) const noexcept {
    if (auto p = memChr(delim); p) {
        before = StringView(data(), p);
        after = StringView(p + 1, end());

        return true;
    }

    return false;
}

u64 StringView::stou() const noexcept {
    u64 result = 0;

    for (size_t i = 0; i < length(); ++i) {
        u8 ch = (*this)[i];

        if (ch >= '0' && ch <= '9') {
            result = result * 10 + (ch - '0');
        }
    }

    return result;
}

template <>
void stl::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, StringView str) {
    out.write(str.data(), str.length());
}
