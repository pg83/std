#pragma once

namespace Std {
    template <typename T>
    inline T& singleton() {
        static T t;

        return t;
    }
}
