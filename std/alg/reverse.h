#pragma once

#include "xchg.h"

namespace stl {
    template <typename B, typename E>
    void reverse(B b, E e) {
        for (; b < e; xchg(*b++, *--e)) {
        }
    }

    template <typename R>
    void reverse(R&& r) {
        reverse(r.begin(), r.end());
    }
}
