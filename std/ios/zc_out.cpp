#include "zc_out.h"
#include "manip.h"

#include <std/sys/crt.h>
#include <std/str/view.h>

using namespace Std;
using namespace Std::Manip;

namespace {
    struct U64 {
        u64 val;
    };

    struct I64 {
        i64 val;
    };
}

ZeroCopyOutput::~ZeroCopyOutput() noexcept {
}

void ZeroCopyOutput::writeImpl(const void* data, size_t len) {
    bump(imbue(len) << StringView((const u8*)data, len));
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
