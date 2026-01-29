#pragma once


namespace Std {
    template <typename T>
    using rem_ref = __remove_reference_t(T);

    // move semantics
    template <typename T>
    constexpr rem_ref<T>&& move(T&& t) noexcept {
        return static_cast<rem_ref<T>&&>(t);
    }

    // perfect forwarding
    template <typename T>
    constexpr T&& forward(rem_ref<T>& t) noexcept {
        return static_cast<T&&>(t);
    }

    template <typename T>
    constexpr T&& forward(rem_ref<T>&& t) noexcept {
        return static_cast<T&&>(t);
    }
}
