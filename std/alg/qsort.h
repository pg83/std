#pragma once

#include "xchg.h"

#include <std/typ/support.h>

namespace Std::QuickSort {
    template <typename I, typename C>
    struct Context {
        C* f;

        inline auto partition(I b, I e, I p) {
            auto c = b;

            for (; b != e; ++b) {
                if ((*f)(*b, *p)) {
                    xchg(*b, *c++);
                }
            }

            xchg(*c, *p);

            return c;
        }

        inline void sort(I b, I e) {
            // already sorted
            if (e - b < 2) {
                return;
            }

            auto p = partition(b, e - 1, e - 1);

            sort(b, p);
            sort(p + 1, e);
        }
    };
}

namespace Std {
    template <typename I, typename C>
    inline void quickSort(I b, I e, C&& f) {
        QuickSort::Context<I, C>{.f = &f}.sort(b, e);
    }

    template <typename I>
    inline void quickSort(I b, I e) {
        return quickSort(b, e, [](const auto& x, const auto& y) {
            return x < y;
        });
    }

    template <typename R, typename C>
    inline void quickSort(R&& r, C&& f) {
        return quickSort(r.begin(), r.end(), forward<C>(f));
    }

    template <typename R>
    inline void quickSort(R&& r) {
        return quickSort(r.begin(), r.end());
    }
}
