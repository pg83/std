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
    void* chunk;
    size_t clen;

    while (auto len = (next(&chunk, &clen), in.read(chunk, clen))) {
        commit(len);
    }
}

void ZeroCopyOutput::recvFromZ(ZeroCopyInput& in) {
    if (in.hint() > hint()) {
        const void* chunk;

        while (auto len = in.next(&chunk)) {
            in.commit(writeP(chunk, len));
        }
    } else {
        recvFromI(in);
    }
}

size_t ZeroCopyOutput::writeImpl(const void* data, size_t len) {
    void* chunk;

    len = min(len, next(&chunk));

    memCpy(chunk, data, len);
    commit(len);

    return len;
}

size_t ZeroCopyOutput::next(void** chunk) {
    size_t res = 1;

    *chunk = imbue(&res);

    return res;
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
