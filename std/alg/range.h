#pragma once

namespace stl {
    template <typename B, typename E>
    struct Range {
        const B b;
        const E e;

        auto begin() const noexcept {
            return b;
        }

        auto end() const noexcept {
            return e;
        }

        unsigned long length() const noexcept {
            return e - b;
        }
    };

    template <typename B, typename E>
    auto range(B b, E e) noexcept {
        return Range<B, E>{b, e};
    }

    template <typename C>
    auto range(const C& c) noexcept {
        return range(c.begin(), c.end());
    }

    template <typename C>
    auto mutRange(C& c) noexcept {
        return range(c.mutBegin(), c.mutEnd());
    }
}
