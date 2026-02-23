#pragma once

namespace Std {
    template <typename T>
    inline void* destruct(T* t) {
        return (t->~T(), (void*)t);
    }
}
