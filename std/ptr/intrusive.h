#pragma once

#include "refcount.h"

namespace Std::IPP {
    template <typename T>
    struct Ops: public RefCountOps<T> {
        static inline auto ptr(const T* t) noexcept {
            return t;
        }

        static inline auto mutPtr(T* t) noexcept {
            return t;
        }
    };
}

namespace Std {
    template <typename T>
    using IntrusivePtr = RefCountPtr<T, IPP::Ops<T>>;
}
