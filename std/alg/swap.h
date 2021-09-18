#pragma once

namespace Std {
    template <typename T>
    inline void swap(T& l, T& r) {
        T t = l;

        l = r;
        r = t;
    }
}
