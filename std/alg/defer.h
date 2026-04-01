#pragma once

#define STD_DEFER ::stl::ScopedGuard _ = [&] mutable -> void

namespace stl {
    template <typename F>
    struct ScopedGuard {
        ScopedGuard(F ff) noexcept
            : f(ff)
        {
        }

        ~ScopedGuard() {
            f();
        }

        F f;
    };

    template <typename P, typename F>
    static void atExit(P* pool, F f) {
        pool->template make<ScopedGuard<F>>(f);
    }
}
