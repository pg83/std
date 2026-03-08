#pragma once

namespace stl {
    template <typename T>
    auto destruct(T* t) {
        return (t->~T(), t);
    }
}
