#pragma once

namespace Std::Meta::Private {
    // select
    template <bool, typename T1, typename T2>
    struct Select {
        using R = T1;
    };

    template <typename T1, typename T2>
    struct Select<false, T1, T2> {
        using R = T2;
    };

    // enable if
    template <bool, typename T>
    struct EnableIf;

    template <typename T>
    struct EnableIf<true, T> {
        using R = T;
    };
}

// common
namespace Std::Meta {
    template <bool V, typename T1, typename T2>
    using Select = typename Private::Select<V, T1, T2>::R;

    template <bool V, typename T>
    using EnableIf = typename Private::EnableIf<V, T>::R;

    template <bool V>
    struct Bool {
        enum {
            R = V
        };
    };
}
