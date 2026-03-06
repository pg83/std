#pragma once

#define STD_DEFER ::stl::ScopedGuard _ = [&] mutable -> void

namespace stl {
    template <typename F>
    struct ScopedGuard {
        inline ScopedGuard(F&& ff) noexcept
            : f(ff)
        {
        }

        inline ~ScopedGuard() {
            f();
        }

        F f;
    };
}
