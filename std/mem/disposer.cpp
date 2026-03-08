#include "disposer.h"

#include <std/alg/exchange.h>
#include <std/alg/destruct.h>

using namespace stl;

void Disposer::dispose() {
    while (end) {
        destruct(exchange(end, end->prev));
    }
}

unsigned Disposer::length() const {
    unsigned res = 0;

    for (auto cur = end; cur; cur = cur->prev) {
        ++res;
    }

    return res;
}
