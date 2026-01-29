#pragma once

namespace Std {
    template <typename T>
    inline T min(T l, T r) {
        return l < r ? l : r;
    }

    template <typename T>
    inline T max(T l, T r) {
        return l < r ? r : l;
    }
}
