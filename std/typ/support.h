#pragma once

#include "traits.h"

namespace Std {
    //move semantics
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
