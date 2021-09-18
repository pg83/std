#pragma once

#include "swap.h"

namespace Std {
    template <typename B, typename E>
    inline void reverse(B b, E e) {
        while (b < e) {
            swap(*b++, *--e);
        }
    }

    template <typename R>
    inline void reverse(R&& r) {
        reverse(r.begin(), r.end());
    }
}
