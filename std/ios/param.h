#pragma once

namespace Std {
    template <bool>
    struct Bool {
        enum {
            Result = true
        };
    };

    template <>
    struct Bool<false> {
        enum {
            Result = false
        };
    };

    namespace Private {
        template <typename T>
        char test(int T::*);

        template <typename T>
        int test(...);
    };

    template <typename T>
    using IsClass = Bool<sizeof(Private::test<T>(nullptr)) == 1>;

    template <typename T, bool>
    struct FuncParamImpl {
        using Result = T;
    };

    template <typename T>
    struct FuncParamImpl<T, false> {
        using Result = const T&;
    };

    template <typename T>
    using FuncParam = typename FuncParamImpl<T, !IsClass<T>::Result && (sizeof(T) <= sizeof(void*))>::Result;
}
