#include "in_zc.h"

#include <std/sys/crt.h>
#include <std/sys/types.h>
#include <std/alg/minmax.h>

using namespace Std;

ZeroCopyInput::~ZeroCopyInput() noexcept {
}

const void* ZeroCopyInput::nextLimited(size_t* len) {
    size_t rlen = *len;
    auto res = next(&rlen);

    return (*len = min(rlen, *len), res);
}

size_t ZeroCopyInput::readImpl(void* data, size_t len) {
    auto chunk = nextLimited(&len);

    memCpy(data, chunk, len);
    commit(len);

    return len;
}

ZeroCopyInput* ZeroCopyInput::zeroCopy() noexcept {
    return this;
}
