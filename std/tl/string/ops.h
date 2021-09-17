#pragma once

#include <std/os/types.h>

namespace Std {
    template <typename B>
    class StringOps {
        // parent access
        inline auto base() noexcept {
            return static_cast<B*>(this);
        }

        inline auto base() const noexcept {
            return static_cast<const B*>(this);
        }

    public:
        // iterator ops
        inline auto begin() const noexcept {
            return base()->data();
        }

        inline auto end() const noexcept {
            return begin() + base()->length();
        }

        inline auto mutBegin() noexcept {
            return base()->data();
        }

        inline auto mutEnd() noexcept {
            return mutBegin() + base()->length();
        }

        inline auto& operator[](size_t i) noexcept {
            return *(begin() + i);
        }

        inline const auto& operator[](size_t i) const noexcept {
            return *(begin() + i);
        }
    };
}
