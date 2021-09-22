#include "xchg.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Xchg) {
    // .xchg
    static bool x1Called = false;

    struct T1 {
        T1() = default;

        T1(const T1&) = delete;
        T1(T1&&) = delete;
        T1& operator=(const T1&) = delete;
        T1& operator=(T1&&) = delete;

        inline void xchg(T1&) noexcept {
            x1Called = true;
        }
    };

    STD_TEST(testXchg) {
        T1 l;
        T1 r;

        xchg(l, r);
        STD_INSIST(x1Called);
    }

    // .swap
    static bool x2Called = false;

    struct T2 {
        T2() = default;

        T2(const T2&) = delete;
        T2(T2&&) = delete;
        T2& operator=(const T2&) = delete;
        T2& operator=(T2&&) = delete;

        inline void swap(T2&) noexcept {
            x2Called = true;
        }
    };

    STD_TEST(testSwap) {
        T2 l;
        T2 r;

        xchg(l, r);
        STD_INSIST(x2Called);
    }

    // move
    static bool x3Called = false;

    struct T3 {
        T3() = default;

        T3(const T3&) = delete;
        T3(T3&&) {
            x3Called = true;
        }
        T3& operator=(const T3&) = delete;
        T3& operator=(T3&&) {
            x3Called = true;

            return *this;
        }
    };

    STD_TEST(testMove) {
        T3 l;
        T3 r;

        xchg(l, r);
        STD_INSIST(x3Called);
    }
}
