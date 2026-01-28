#pragma once

namespace Std {
    template <typename B, typename E>
    struct Range {
        const B b;
        const E e;

        inline auto begin() const noexcept {
            return b;
        }

        inline auto end() const noexcept {
            return e;
        }

        inline unsigned long length() const noexcept {
            return e - b;
        }
    };

    template <typename B, typename E>
    inline auto range(B b, E e) noexcept {
        return Range<B, E>{b, e};
    }

    template <typename C>
    inline auto range(const C& c) noexcept {
        return range(c.begin(), c.end());
    }

    template <typename C>
    inline auto mutRange(C& c) noexcept {
        return range(c.mutBegin(), c.mutEnd());
    }
}
