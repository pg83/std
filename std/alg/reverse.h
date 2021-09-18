#pragma once

#include "swap.h"

namespace Std {
    template <typename B, typename E>
    void reverse(B b, E e) {
        while (b < e) {
            swap(*b++, *--e);
        }
    }

    template <typename R>
    void reverse(R&& r) {
        reverse(r.begin(), r.end());
    }
}
