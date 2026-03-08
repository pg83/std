#pragma once

#include "refcount.h"

#include <std/mem/embed.h>
#include <std/typ/support.h>

namespace stl::SPP {
    template <typename T>
    struct Ops: public RefCountOps<T> {
        static auto ptr(const T* t) noexcept {
            return &t->t;
        }

        static auto mutPtr(T* t) noexcept {
            return &t->t;
        }
    };

    template <typename T, typename R>
    struct Base: public Embed<T>, public R {
        using Embed<T>::Embed;
    };
}

namespace stl {
    class ARC;

    template <typename T, typename R>
    using SharedPtr = RefCountPtr<SPP::Base<T, R>, SPP::Ops<SPP::Base<T, R>>>;

    template <typename T>
    using AtomicSharedPtr = SharedPtr<T, ARC>;
}
