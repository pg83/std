#pragma once

namespace stl {
    template <typename T>
    T min(T l, T r) {
        return l < r ? l : r;
    }

    template <typename T>
    T max(T l, T r) {
        return l < r ? r : l;
    }
}
