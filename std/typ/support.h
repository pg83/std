#pragma once

#define rem_ref(T) __remove_reference_t(T)

namespace Std {
    // move semantics
    template <typename T>
    constexpr rem_ref(T)&& move(T&& t) noexcept {
        return static_cast<rem_ref(T)&&>(t);
    }

    // perfect forwarding
    template <typename T>
    constexpr T&& forward(rem_ref(T)& t) noexcept {
        return static_cast<T&&>(t);
    }

    template <typename T>
    constexpr T&& forward(rem_ref(T)&& t) noexcept {
        return static_cast<T&&>(t);
    }
}
