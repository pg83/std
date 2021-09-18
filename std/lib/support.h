#pragma once

namespace Std {
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

    template <typename T>
    using RemoveReference = typename RemoveReferenceHelper<T>::R;

    //move semantics
    template <typename T>
    constexpr RemoveReference<T>&& move(T&& t) noexcept {
        return static_cast<RemoveReference<T>&&>(t);
    }
}
