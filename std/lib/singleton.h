#pragma once

namespace Std {
    template <typename T>
    inline T& singleton() {
        static T* t = new T();

        return *t;
    }
}
