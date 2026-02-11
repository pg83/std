#include "in_zc.h"

#include <std/sys/crt.h>
#include <std/sys/types.h>
#include <std/alg/minmax.h>

using namespace Std;

ZeroCopyInput::~ZeroCopyInput() noexcept {
}

size_t ZeroCopyInput::readImpl(void* data, size_t len) {
    size_t rlen = len;
    auto chunk = next(&rlen);
    const size_t res = min(len, rlen);

    memCpy(data, chunk, res);
    commit(res);

    return res;
}
