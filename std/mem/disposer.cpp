#include "disposer.h"

#include <std/alg/exchange.h>
#include <std/alg/destruct.h>

using namespace Std;

void Disposer::dispose() noexcept {
    while (end) {
        destruct(exchange(end, end->prev));
    }
}

unsigned Disposer::length() const noexcept {
    unsigned res = 0;

    for (auto cur = end; cur; cur = cur->prev) {
        ++res;
    }

    return res;
}
