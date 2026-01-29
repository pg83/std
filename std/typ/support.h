#pragma once

namespace Std::Traits {
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
}

namespace Std {
    // move semantics
    template <typename T>
    constexpr Traits::RemoveReference<T>&& move(T&& t) noexcept {
        return static_cast<Traits::RemoveReference<T>&&>(t);
    }

    // perfect forwarding
    template <typename T>
    constexpr T&& forward(Traits::RemoveReference<T>& t) noexcept {
        return static_cast<T&&>(t);
    }

    template <typename T>
    constexpr T&& forward(Traits::RemoveReference<T>&& t) noexcept {
        return static_cast<T&&>(t);
    }
}
