#pragma once

namespace Std {
    template <typename T>
    inline void swap(T& l, T& r) {
        T t = l;

        l = r;
        r = t;
    }

    // remove reference
    template <typename T>
    struct RemoveReferenceHelper {
        using Result = T;
    };

    template <typename T>
    struct RemoveReferenceHelper<T&> {
        using Result = T;
    };

    template <typename T>
    struct RemoveReferenceHelper<T&&> {
        using Result = T;
    };

    template <typename T>
    using RemoveReference = typename RemoveReferenceHelper<T>::Result;

    //move semantics
    template <typename T>
    constexpr RemoveReference<T>&& move(T&& t) noexcept {
        return static_cast<RemoveReference<T>&&>(t);
    }
}
