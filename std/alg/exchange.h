#pragma once

namespace stl {
    template <typename T, typename N>
    T exchange(T& o, N n) {
        auto r = o;

        o = n;

        return r;
    }
}
