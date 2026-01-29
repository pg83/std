#include "disposer.h"

#include <std/alg/exchange.h>

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }
}

void Disposer::dispose() noexcept {
    IntrusiveList tmp;

    lst.xchgWithEmptyList(tmp);

    for (auto end = tmp.end(), cur = (const IntrusiveNode*)end->prev; cur != end;) {
        destruct((Disposable*)exchange(cur, cur->prev));
    }
}
