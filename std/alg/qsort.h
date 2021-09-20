#pragma once

#include "xchg.h"

#include <std/typ/support.h>

namespace Std {
    template <typename I, typename C>
    inline auto Partition(I b, I e, I p, C&& f) {
        auto c = b;

        for (; b != e; ++b) {
            if (f(*b, *p)) {
                xchg(*b, *c++);
            }
        }

        xchg(*c, *p);

        return c;
    }

    template <typename I, typename C>
    inline void QuickSort(I b, I e, C&& f) {
        // already sorted
        if (e - b < 2) {
            return;
        }

        auto p = Partition(b, e - 1, e - 1, forward<C>(f));

        QuickSort(b, p, forward<C>(f));
        QuickSort(p + 1, e, forward<C>(f));
    }

    template <typename I>
    inline void QuickSort(I b, I e) {
        return QuickSort(b, e, [](const auto& x, const auto& y) {
            return x < y;
        });
    }

    template <typename R, typename C>
    inline void QuickSort(R&& r, C&& f) {
        return QuickSort(r.begin(), r.end(), forward<C>(f));
    }

    template <typename R>
    inline void QuickSort(R&& r) {
        return QuickSort(r.begin(), r.end());
    }
}
