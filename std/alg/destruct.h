#pragma once

namespace Std {
    template <typename T>
    inline void destruct(T* t) {
        t->~T();
    }
}
