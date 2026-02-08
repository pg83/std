#include "copy.h"
#include "input.h"
#include "zc_out.h"
#include "out_buf.h"

#include <std/alg/minmax.h>
#include <std/alg/advance.h>

void Std::copy(Input& in, Output& out) {
    OutBuf ob(out);
    zeroCopy(in, ob);
}

void Std::zeroCopy(Input& in, ZeroCopyOutput& out) {
    size_t chunkSize = 128;
    bool hinted = out.hint(&chunkSize);

    while (true) {
        size_t bufLen;

        void* ptr = out.imbue(chunkSize, &bufLen);
        const size_t len = in.readP(ptr, bufLen);

        if (!len) {
            return;
        }

        out.bump(advancePtr(ptr, len));

        if (!hinted) {
            chunkSize = min<size_t>(chunkSize * 2, 1 << 16);
        }
    }
}
