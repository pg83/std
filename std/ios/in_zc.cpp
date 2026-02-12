#include "in_zc.h"
#include "output.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>

using namespace Std;

ZeroCopyInput::~ZeroCopyInput() noexcept {
}

size_t ZeroCopyInput::readImpl(void* data, size_t len) {
    const void* chunk;

    len = min(next(&chunk), len);

    memCpy(data, chunk, len);
    commit(len);

    return len;
}

void ZeroCopyInput::sendTo(Output& out) {
    out.recvFromZ(*this);
}

void ZeroCopyInput::readLine(Buffer& buf) {
    const void* chunk;

    while (auto len = next(&chunk)) {
        StringView part((const u8*)chunk, len);

        if (auto pos = part.memChr('\n'); pos) {
            const auto plen = pos - part.begin();

            buf.append(part.begin(), plen);
            commit(plen + 1);

            return;
        } else {
            buf.append(part.begin(), part.length());
            commit(part.length());
        }
    }
}
