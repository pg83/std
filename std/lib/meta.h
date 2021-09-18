#pragma once

namespace Std::Meta {
    template <bool, typename T1, typename T2>
    struct SelectImpl {
        using R = T1;
    };

    template <typename T1, typename T2>
    struct SelectImpl<false, T1, T2> {
        using R = T2;
    };

    template <bool V, typename T1, typename T2>
    using Select = typename SelectImpl<V, T1, T2>::R;
}
