#pragma once

namespace stl {
    template <typename T>
    void* destruct(T* t) {
        return (t->~T(), (void*)t);
    }
}
