#include "zc_out.h"
#include "manip.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/exchange.h>

using namespace Std;
using namespace Std::Manip;

namespace {
    static constexpr size_t PART = 64 * 1024;

    struct U64 {
        u64 val;
    };

    struct I64 {
        i64 val;
    };
}

ZeroCopyOutput::~ZeroCopyOutput() {
}

void ZeroCopyOutput::writePart(StringView part) {
    bump(imbue(part.length()) << part);
}

void ZeroCopyOutput::writeImpl(const void* data, size_t len) {
    const u8* b = (u8*)data;
    const u8* e = b + len;

    while (true) {
        if (const auto left = e - b; left <= PART) {
            writePart(StringView(b, left));

            return;
        }

        writePart(StringView(exchange(b, b + PART), PART));
    }
}

// std types
#define DEF_OUT(typ)                                                                       \
    template <>                                                                            \
    void Std::output<ZeroCopyOutput, typ>(ZeroCopyOutput & out, typ v) {                   \
        out << I64{v};                                                                     \
    }                                                                                      \
    template <>                                                                            \
    void Std::output<ZeroCopyOutput, unsigned typ>(ZeroCopyOutput & out, unsigned typ v) { \
        out << U64{v};                                                                     \
    }

DEF_OUT(int)
DEF_OUT(long)
DEF_OUT(short)
DEF_OUT(long long)

template <>
void Std::output<ZeroCopyOutput, U64>(ZeroCopyOutput& out, U64 v) {
    out.bump(out.imbue(24) << v.val);
}

template <>
void Std::output<ZeroCopyOutput, I64>(ZeroCopyOutput& out, I64 v) {
    out.bump(out.imbue(24) << v.val);
}

template <>
void Std::output<ZeroCopyOutput, const char*>(ZeroCopyOutput& out, const char* v) {
    out << StringView(v);
}
