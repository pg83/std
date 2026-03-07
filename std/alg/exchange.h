#pragma once

namespace stl {
    template <typename T, typename N>
    inline T exchange(T& o, N n) {
        auto r = o;

        o = n;

        return r;
    }
}
