#include "num.h"

#include "view.h"

#include <stdlib.h>

using namespace stl;

namespace {
    static bool digitValue(u8 byte, unsigned base, u64& value) {
        unsigned digit;

        if (byte >= '0' && byte <= '9') {
            digit = byte - '0';
        } else if (byte >= 'a' && byte <= 'z') {
            digit = byte - 'a' + 10;
        } else if (byte >= 'A' && byte <= 'Z') {
            digit = byte - 'A' + 10;
        } else {
            return false;
        }

        if (digit >= base) {
            return false;
        }

        value = digit;

        return true;
    }

    static bool accumulate(StringView digits, unsigned base, u64 limit, u64& out) {
        if (digits.empty()) {
            return false;
        }

        u64 value = 0;

        for (const u8 byte : digits) {
            u64 digit;

            if (!digitValue(byte, base, digit)) {
                return false;
            }

            if (value > (limit - digit) / base) {
                return false;
            }

            value = value * base + digit;
        }

        out = value;

        return true;
    }
}

bool stl::parseI64(StringView text, i64& out) {
    bool negative = false;

    if (!text.empty() && (text[0] == '-' || text[0] == '+')) {
        negative = text[0] == '-';
        text = text.suffix(text.length() - 1);
    }

    const u64 limit = negative ? (u64)(1) << 63 : ((u64)(1) << 63) - 1;
    u64 value;

    if (!accumulate(text, 10, limit, value)) {
        return false;
    }

    out = negative ? (i64)(0 - value) : (i64)(value);

    return true;
}

bool stl::parseU64(StringView text, u64& out, unsigned base) {
    if (!text.empty() && text[0] == '+') {
        text = text.suffix(text.length() - 1);
    }

    return accumulate(text, base, ~(u64)(0), out);
}

bool stl::parseF64(StringView text, double& out) {
    // The exponent grammar is not worth reimplementing; libc parses a
    // bounded NUL copy of the view.
    char bounded[64];

    if (text.empty() || text.length() >= sizeof(bounded)) {
        return false;
    }

    for (size_t index = 0; index < text.length(); ++index) {
        bounded[index] = (char)(text[index]);
    }

    bounded[text.length()] = '\0';

    char* end = nullptr;
    const double value = strtod(bounded, &end);

    if (end != bounded + text.length()) {
        return false;
    }

    out = value;

    return true;
}
