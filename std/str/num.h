#pragma once

#include <std/sys/types.h>

namespace stl {
    class StringView;

    // Whole-view numeric parses: an optional sign, then digits of the
    // base, nothing else. Empty input, stray bytes, and overflow all
    // report false and leave out untouched.
    bool parseI64(StringView text, i64& out);
    bool parseU64(StringView text, u64& out, unsigned base = 10);
    bool parseF64(StringView text, double& out);
}
