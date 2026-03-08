#pragma once

namespace stl {
    template <typename T>
    struct ScopedPtr {
        T* ptr;

        void drop() {
            ptr = 0;
        }

        ~ScopedPtr() {
            delete ptr;
        }
    };
}
