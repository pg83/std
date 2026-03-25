#include "disposer.h"

#include <std/alg/exchange.h>
#include <std/alg/destruct.h>

using namespace stl;

void Disposer::dispose() noexcept {
    auto cur = exchange(end, (Disposable*)0);

    while (cur) {
        destruct(exchange(cur, cur->prev));
    }
}

unsigned Disposer::length() const noexcept {
    unsigned res = 0;

    for (auto cur = end; cur; cur = cur->prev) {
        ++res;
    }

    return res;
}
