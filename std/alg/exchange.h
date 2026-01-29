#pragma once

namespace Std {
    template <typename T, typename N>
    inline T exchange(T& o, N n) noexcept {
        auto r = o;

        o = n;

        return r;
    }
}
