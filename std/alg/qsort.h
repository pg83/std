#pragma once

#include "xchg.h"

#include <std/rng/pcg.h>
#include <std/typ/support.h>

#include <initializer_list>

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
            for (auto i = b + 1; i != e; ++i) {
                for (auto j = i; j != b && f(*j, *(j - 1)); --j) {
                    xchg(*j, *(j - 1));
                }
            }
        }

        inline void shellSort(I b, I e) {
            for (auto gap : {57, 23, 10, 4, 1}) {
                for (auto i = b + gap; i < e; i += gap) {
                    for (auto j = i; (j >= (b + gap)) && f(*j, *(j - gap)); j -= gap) {
                        xchg(*j, *(j - gap));
                    }
                }
            }
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

        inline auto partitionLomuto(I b, I e) {
            const auto& p = *e;
            auto c = b;

            for (; b != e; ++b) {
                if (f(*b, p)) {
                    xchg(*b, *c++);
                }
            }

            return c;
        }

        inline auto partitionHoare(I b, I e) {
            auto p = e;

            while (true) {
                while (b != e && f(*b,  *p)) {
                    ++b;
                }

                // TODO
                while (b != e && *(e - 1) >= *p) {
                    --e;
                }

                if (b == e) {
                    return b;
                }

                xchg(*b, *(e - 1));
            }
        }

        inline void qSort(I b, I e) {
            auto len = e - b;

            if (len < 2) {
                // already sorted
                return;
            }

            if (__builtin_expect(len < 64, 0)) {
                return insertionSort(b, e);
            }

            auto l = e - 1;

            // pivot to last
            xchg(*choosePivot(b, e), *l);

            // place for pivot
            auto p = partitionLomuto(b, l);

            // move pivot form last to proper place
            xchg(*p, *l);

            // recurse
            qSort(b, p);
            qSort(p + 1, e);
        }
    };
}

namespace Std {
    template <typename I, typename C>
    inline void quickSort(I b, I e, C&& f) {
        QSP::Context<I, C>(f).qSort(b, e);
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
