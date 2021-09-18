#pragma once

namespace Std::Meta {
    template <typename T>
    struct PassByValue {
        enum {
            R = sizeof(T) <= sizeof(void*) && __is_trivially_copyable(T)
        };
    };

    template <bool, typename T1, typename T2>
    struct Select {
        using R = T1;
    };

    template <typename T1, typename T2>
    struct Select<false, T1, T2> {
        using R = T2;
    };

    template <typename T>
    using FuncParam = typename Select<PassByValue<T>::R, T, const T&>::R;
}

namespace Std {
    using Meta::FuncParam;
}
