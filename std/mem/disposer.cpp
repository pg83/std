#include "disposer.h"

using namespace Std;

namespace {
    template <typename T>
    inline void destruct(T* t) noexcept {
        t->~T();
    }
}

Disposer::~Disposer() noexcept {
    while (!lst.empty()) {
        destruct((Disposable*)lst.popBack());
    }
}
