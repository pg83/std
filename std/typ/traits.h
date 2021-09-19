#pragma once

#include "meta.h"

namespace Std::Traits::Private {
    using Meta::Bool;

    // remove reference
    template <typename T>
    struct RemoveReferenceHelper {
        using R = T;
    };

    template <typename T>
    struct RemoveReferenceHelper<T&> {
        using R = T;
    };

    template <typename T>
    struct RemoveReferenceHelper<T&&> {
        using R = T;
    };

    // how to pass function params
    template <typename T>
    struct PassByValue: public Bool<sizeof(T) <= sizeof(void*) && __is_trivially_copyable(T)> {
    };
}

namespace Std::Traits {
    template <typename T>
    using FuncParam = Meta::Select<Private::PassByValue<T>::R, T, const T&>;

    template <typename T>
    using RemoveReference = typename Private::RemoveReferenceHelper<T>::R;

    template <typename B, typename D>
    struct IsBaseOf: public Meta::Bool<__is_base_of(B, D)> {
    };

    template <typename C>
    struct IsClass: public Meta::Bool<__is_class(C)> {
    };
}
