#pragma once

namespace Std {
    template <typename T>
    inline void xchg(T& l, T& r) noexcept {
        T t = l;

        l = r;
        r = t;
    }
}
