#pragma once

#include "meta.h"
#include "intrin.h"

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
}

namespace Std::Traits {
    template <typename T>
    using RemoveReference = typename Private::RemoveReferenceHelper<T>::R;

    template <typename B, typename D>
    struct IsBaseOf: public Meta::Bool<stdIsBaseOf(B, D)> {
    };

    template <typename C>
    struct IsClass: public Meta::Bool<stdIsClass(C)> {
    };

    template <typename T>
    struct HasDestructor: public Meta::Bool<!stdHasTrivialDestructor(T)> {
    };
}
