#include "disposer.h"

#include <std/typ/support.h>
#include <std/alg/destruct.h>
#include <std/alg/exchange.h>

using namespace Std;

void Disposer::dispose() noexcept {
    IntrusiveList tmp(move(lst));

    for (auto end = tmp.end(), cur = (const IntrusiveNode*)end->prev; cur != end;) {
        destruct((Disposable*)exchange(cur, cur->prev));
    }
}
