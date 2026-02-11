#include "copy.h"
#include "input.h"
#include "in_zc.h"
#include "out_zc.h"
#include "out_buf.h"

#include <std/alg/minmax.h>
#include <std/alg/advance.h>

void Std::copy(Input& in, Output& out) {
    if (auto zo = out.zeroCopy(); zo) {
        if (auto zi = in.zeroCopy(); zi) {
            copyZZ(*zi, *zo);
        } else {
            copyIZ(in, *zo);
        }
    } else {
        if (auto zi = in.zeroCopy(); zi) {
            copyZO(*zi, out);
        } else {
            copyIO(in, out);
        }
    }
}

void Std::copyIO(Input& in, Output& out) {
    OutBuf ob(out);
    copyIZ(in, ob);
}

void Std::copyIZ(Input& in, ZeroCopyOutput& out) {
    size_t chunkSize = 128;
    bool hinted = out.hint(&chunkSize);

    while (true) {
        size_t bufLen = chunkSize;

        void* ptr = out.imbue(&bufLen);
        const size_t len = in.read(ptr, bufLen);

        if (!len) {
            return;
        }

        out.commit(advancePtr(ptr, len));

        if (!hinted) {
            chunkSize = min<size_t>(chunkSize * 2, 1 << 16);
        }
    }
}

void Std::copyZO(ZeroCopyInput& in, Output& out) {
    const void* chunk;

    while (auto len = in.next(&chunk)) {
        in.commit(out.writeP(chunk, len));
    }
}

void Std::copyZZ(ZeroCopyInput& in, ZeroCopyOutput& out) {
    // assume passthrough read()
    copyIZ(in, out);
}
