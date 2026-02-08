#pragma once

#define STD_DEFER ::Std::ScopedGuard _ = [&] mutable -> void

namespace Std {
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
