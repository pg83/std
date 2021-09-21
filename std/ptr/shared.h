#pragma once

#include "refcount.h"

#include <std/typ/support.h>

namespace Std::SPP {
    template <typename T>
    struct Ops: public RefCountOps<T> {
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

        template <typename... A>
        inline Base(A&&... a)
            : t(forward<A>(a)...)
        {
        }
    };
}

namespace Std {
    class ARC;

    template <typename T, typename R>
    using SharedPtr = RefCountPtr<SPP::Base<T, R>, SPP::Ops<SPP::Base<T, R>>>;

    template <typename T>
    using AtomicSharedPtr = SharedPtr<T, ARC>;
}
