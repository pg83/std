#pragma once

namespace Std::Private {
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

namespace Std {
    template <typename T>
    using RemoveReference = typename Private::RemoveReferenceHelper<T>::R;

    //move semantics
    template <typename T>
    constexpr RemoveReference<T>&& move(T&& t) noexcept {
        return static_cast<RemoveReference<T>&&>(t);
    }

    template <typename T>
    constexpr T&& forward(T&& t) noexcept {
        return static_cast<T&&>(t);
    }
}
