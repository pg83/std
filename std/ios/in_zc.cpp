#include "in_zc.h"

#include <std/sys/crt.h>
#include <std/sys/types.h>
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

ZeroCopyInput* ZeroCopyInput::zeroCopy() noexcept {
    return this;
}
