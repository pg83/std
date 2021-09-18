#pragma once

namespace Std {
    template <typename T>
    inline T clp2(T value) noexcept {
        T ret = 1;

        while (ret < value) {
            ret *= 2;
        }

        return ret;
    }
}
