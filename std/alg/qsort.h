#pragma once

#include "xchg.h"

#include <std/rng/pcg.h>
#include <std/lib/vector.h>
#include <std/typ/support.h>

namespace Std::QSP {
    template <typename I, typename C>
    struct Context {
        struct WorkItem {
            I b;
            I e;
        };

        C& f;
        PCG32 r;
        Vector<WorkItem> w;

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

        inline auto partitionHoare(I b, I e) {
            for (auto p = e; ; ++b) {
                while (b != e && f(*b , *p)) {
                    ++b;
                }

                while (b != e && f(*p, *--e)) {
                }

                if (b == e) {
                    return b;
                }

                xchg(*b, *e);
            }
        }

        // assume b < e
        inline bool alreadySorted(I b, I e) {
            for (++b; b != e; ++b) {
                if (f(*b, *(b - 1))) {
                    return false;
                }
            }

            return true;
        }

        inline void qSortStep(I b, I e) {
            if (e - b < 16) {
                return;
            }

            if (alreadySorted(b, e)) {
                return;
            }

            auto l = e - 1;

            // pivot to last
            xchg(*choosePivot(b, e), *l);

            // place for pivot
            auto p = partitionHoare(b, l);

            // move pivot form last to proper place
            xchg(*p, *l);

            // recurse
            w.pushBack(WorkItem{p + 1, e});
            w.pushBack(WorkItem{b, p});
        }

        inline void qSortLoop() {
            while (!w.empty()) {
                auto item = w.popBack();

                qSortStep(item.b, item.e);
            }
        }

        inline void qSort(I b, I e) {
            if (b != e) {
                qSortStep(b, e);
                qSortLoop();
                insertionSort(b, e);
            }
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
