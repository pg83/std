#pragma once

#include "refcount.h"

namespace Std::SPP {
    template <typename T>
    struct Ops {
        static inline auto ref(T* t) noexcept {
            t->ref();
        }

        static inline auto unref(T* t) noexcept {
            if (t->refCount() == 1 || t->unref() == 0) {
                delete t;
            }
        }

        static inline auto ptr(const T* t) noexcept {
            return &t->t;
        }

        static inline auto mutPtr(T* t) noexcept {
            return &t->t;
        }
    };

    template <typename T, typename R>
    struct Base: public R {
        T t;
    };
}

namespace Std {
    class ARC;

    template <typename T, typename R>
    using SharedPtr = RefCountPtr<SPP::Base<T, R>, SPP::Ops<SPP::Base<T, R>>>;

    template <typename T>
    using AtomicSharedPtr = SharedPtr<T, ARC>;
}
