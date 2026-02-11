#include "copy.h"
#include "input.h"
#include "out_zc.h"
#include "out_buf.h"

#include <std/alg/minmax.h>
#include <std/alg/advance.h>

void Std::copy(Input& in, Output& out) {
    if (auto zc = out.zeroCopy(); zc) {
        zeroCopy(in, *zc);
    } else {
        copyCopy(in, out);
    }
}

void Std::copyCopy(Input& in, Output& out) {
    OutBuf ob(out);
    zeroCopy(in, ob);
}

void Std::zeroCopy(Input& in, ZeroCopyOutput& out) {
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
