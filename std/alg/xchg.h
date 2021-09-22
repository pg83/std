#pragma once

#include <std/typ/support.h>

namespace Std {
    template <typename T>
    inline void xchg(T& l, T& r) noexcept {
        if constexpr (requires {l.xchg(r);}) {
            l.xchg(r);
        } else if constexpr (requires {l.swap(r);}) {
            l.swap(r);
        } else if constexpr (requires {T(move(l)) = move(l);}) {
            T t = move(l);

            l = move(r);
            r = move(t);
        } else {
            T t = l;

            l = r;
            r = t;
        }
    }
}
