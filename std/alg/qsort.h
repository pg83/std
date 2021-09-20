#pragma once

#include "xchg.h"

#include <std/rng/pcg.h>
#include <std/typ/support.h>

namespace Std::QSP {
    template <typename I, typename C>
    struct Context {
        C& f;
        PCG32 r;

        inline Context(C& _f) noexcept
            : f(_f)
            , r((size_t)&f) // use address as seed
        {
        }

        inline void insertionSort(I b, I e) {
            for (auto i = b + 1; i < e; ++i) {
                for (auto j = i; j > b && f(*j, *(j - 1)); --j) {
                    xchg(*j, *(j - 1));
                }
            }
        }

        inline void shellSort(I b, I e) {
        }

        inline void sortIt(I& a, I& b) {
            if (f(*a, *b)) {
                xchg(a, b);
            }
        }

        inline auto median(I a, I b, I c) {
            sortIt(a, b);
            sortIt(b, c);
            sortIt(a, b);

            return b;
        }

        inline auto chooseRandom(I b, I e) noexcept {
            return b + r.nextU32() % (e - b);
        }

        inline auto choosePivot(I b, I e) {
            return median(b, chooseRandom(b + 1, e - 1), e - 1);
        }

        inline auto partition(I b, I e, I p) {
            auto c = b;

            for (; b != e; ++b) {
                if (f(*b, *p)) {
                    xchg(*b, *c++);
                }
            }

            xchg(*c, *p);

            return c;
        }

        inline void sort(I b, I e) {
            // already sorted
            auto len = e - b;

            if (len < 2) {
                return;
            }

            if (len < 32) {
                insertionSort(b, e);
            } else {
                xchg(*choosePivot(b, e), *(e - 1));

                auto p = partition(b, e - 1, e - 1);

                sort(b, p);
                sort(p + 1, e);
            }
        }
    };
}

namespace Std {
    template <typename I, typename C>
    inline void quickSort(I b, I e, C&& f) {
        QSP::Context<I, C>(f).sort(b, e);
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
