#pragma once

#include "refcount.h"

namespace Std {
    template <typename T>
    struct IntrusiveOps {
        static inline auto ref(T* t) noexcept {
            t->ref();
        }

        static inline auto unref(T* t) noexcept {
            if (t->refCount() == 1 || t->unref() == 0) {
                delete t;
            }
        }

        static inline auto ptr(const T* t) noexcept {
            return t;
        }

        static inline auto mutPtr(T* t) noexcept {
            return t;
        }
    };

    template <typename T>
    using IntrusivePtr = RefCountPtr<T, IntrusiveOps<T>>;
}
