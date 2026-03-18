#include "future.h"

#include <std/alg/exchange.h>

using namespace stl;

void* Future::release() noexcept {
    return exchange(value_, nullptr);
}
