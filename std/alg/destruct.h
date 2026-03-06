#pragma once

namespace stl {
    template <typename T>
    inline void* destruct(T* t) {
        return (t->~T(), (void*)t);
    }
}
