#include "in_zc.h"
#include "output.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>

using namespace stl;

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

bool ZeroCopyInput::readLine(Buffer& buf) {
    return readTo(buf, u8'\n');
}

bool ZeroCopyInput::readTo(Buffer& buf, u8 delim) {
    const void* chunk;

    size_t len = next(&chunk);

    if (!len) {
        return false;
    }

    do {
        StringView part((const u8*)chunk, len);

        if (auto pos = part.memChr(delim); pos) {
            StringView line(part.begin(), pos);

            buf.append(line.begin(), line.length());
            commit(line.length() + 1);

            return true;
        } else {
            buf.append(part.begin(), part.length());
            commit(part.length());
        }
    } while ((len = next(&chunk)));

    return true;
}

void ZeroCopyInput::drain() {
    const void* chunk;
    size_t n;

    while ((n = next(&chunk))) {
        commit(n);
    }
}
