#pragma once

namespace stl {
    template <typename B, typename E>
    struct Range {
        const B b;
        const E e;

        auto begin() const {
            return b;
        }

        auto end() const {
            return e;
        }

        unsigned long length() const {
            return e - b;
        }
    };

    template <typename B, typename E>
    auto range(B b, E e) {
        return Range<B, E>{b, e};
    }

    template <typename C>
    auto range(const C& c) {
        return range(c.begin(), c.end());
    }

    template <typename C>
    auto mutRange(C& c) {
        return range(c.mutBegin(), c.mutEnd());
    }
}
