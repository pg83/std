#pragma once

namespace Std::Meta {
    namespace Private {
        template <bool, typename T1, typename T2>
        struct Select {
            using R = T1;
        };

        template <typename T1, typename T2>
        struct Select<false, T1, T2> {
            using R = T2;
        };
    }

    template <bool V, typename T1, typename T2>
    using Select = typename Private::Select<V, T1, T2>::R;

    namespace Private {
        template <bool, typename T>
        struct EnableIf;

        template <typename T>
        struct EnableIf<true, T> {
            using R = T;
        };
    }

    template <bool V, typename T>
    using EnableIf = typename Private::EnableIf<V, T>::R;
}
