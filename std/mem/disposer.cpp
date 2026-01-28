#include "disposer.h"

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }
}

void Disposer::dispose() noexcept {
    while (!lst.empty()) {
        destruct((Disposable*)lst.popBack());
    }
}
