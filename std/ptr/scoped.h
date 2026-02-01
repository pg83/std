#pragma once

namespace Std {
    template <typename T>
    struct ScopedPtr {
        T* ptr;

        inline void drop() noexcept {
            ptr = 0;
        }

        inline ~ScopedPtr() {
            delete ptr;
        }
    };
}
