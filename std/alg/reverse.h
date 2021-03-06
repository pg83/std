#pragma once

#include "xchg.h"

namespace Std {
    template <typename B, typename E>
    inline void reverse(B b, E e) {
        for (; b < e; xchg(*b++, *--e)) {
        }
    }

    template <typename R>
    inline void reverse(R&& r) {
        reverse(r.begin(), r.end());
    }
}
