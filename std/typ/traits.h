#pragma once

#include "intrin.h"

namespace Std::Traits::Private {
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
}
