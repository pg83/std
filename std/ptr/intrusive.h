#pragma once

#include "refcount.h"

namespace stl::IPP {
    template <typename T>
    struct Ops: public RefCountOps<T> {
        static auto ptr(const T* t) {
            return t;
        }

        static auto mutPtr(T* t) {
            return t;
        }
    };
}

namespace stl {
    template <typename T>
    using IntrusivePtr = RefCountPtr<T, IPP::Ops<T>>;
}
