#pragma once

namespace Std {
    template <typename T>
    inline void xchg(T& l, T& r) noexcept {
        if constexpr (__is_class(T)) {
            l.xchg(r);
        } else {
            T t = l;

            l = r;
            r = t;
        }
    }
}
