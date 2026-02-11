#include "out_zc.h"
#include "manip.h"
#include "input.h"
#include "in_zc.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/minmax.h>
#include <std/alg/advance.h>

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

void ZeroCopyOutput::recvFromI(Input& in) {
    size_t chunkSize = 128;
    bool hinted = hint(&chunkSize);

    while (true) {
        size_t bufLen = chunkSize;

        void* ptr = imbue(&bufLen);
        const size_t len = in.read(ptr, bufLen);

        if (!len) {
            return;
        }

        commit(len);

        if (!hinted) {
            chunkSize = min<size_t>(chunkSize * 2, 1 << 16);
        }
    }
}

void ZeroCopyOutput::recvFromZ(ZeroCopyInput& in) {
    recvFromI(in);
}

size_t ZeroCopyOutput::writeImpl(const void* data, size_t len) {
    auto buf = imbue(len);

    return (commit(buf.distance(buf << StringView((const u8*)data, len))), len);
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

#define DEF_OUT_FLOAT(typ)                                               \
    template <>                                                          \
    void Std::output<ZeroCopyOutput, typ>(ZeroCopyOutput & out, typ v) { \
        auto buf = out.imbue(128);                                       \
        out.commit(buf.distance(buf << (long double)v));                 \
    }

DEF_OUT_FLOAT(float)
DEF_OUT_FLOAT(double)
DEF_OUT_FLOAT(long double)

template <>
void Std::output<ZeroCopyOutput, U64>(ZeroCopyOutput& out, U64 v) {
    auto buf = out.imbue(24);

    out.commit(buf.distance(buf << v.val));
}

template <>
void Std::output<ZeroCopyOutput, I64>(ZeroCopyOutput& out, I64 v) {
    auto buf = out.imbue(24);

    out.commit(buf.distance(buf << v.val));
}

template <>
void Std::output<ZeroCopyOutput, const char*>(ZeroCopyOutput& out, const char* v) {
    out << StringView(v);
}
