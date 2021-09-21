#pragma once

#include <std/sys/types.h>

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
        inline auto length() const noexcept {
            return base()->length();
        }

        inline auto mutData() noexcept {
            return base()->mutData();
        }

        inline auto data() const noexcept {
            return base()->data();
        }

        // iterator ops
        inline auto begin() const noexcept {
            return base()->data();
        }

        inline auto end() const noexcept {
            return begin() + base()->length();
        }

        inline auto mutBegin() noexcept {
            return mutData();
        }

        inline auto mutEnd() noexcept {
            return mutBegin() + base()->length();
        }

        // array ops
        inline auto& mut(size_t i) noexcept {
            return *(mutBegin() + i);
        }

        inline const auto& operator[](size_t i) const noexcept {
            return *(begin() + i);
        }

        inline bool empty() const noexcept {
            return base()->length() == 0;
        }

        inline const auto& back() const noexcept {
            return *(end() - 1);
        }
    };

    int spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept;

    // compare ops
    template <typename L, typename R>
    inline int spaceship(const StringOps<L>& l, const StringOps<R>& r) noexcept {
        return spaceship(l.data(), l.length(), r.data(), r.length());
    }

    template <typename L, typename R>
    inline bool operator==(const StringOps<L>& l, const StringOps<R>& r) noexcept {
        return spaceship(l, r) == 0;
    }

    template <typename L, typename R>
    inline bool operator!=(const StringOps<L>& l, const StringOps<R>& r) noexcept {
        return !(l == r);
    }

    template <typename L, typename R>
    inline bool operator<(const StringOps<L>& l, const StringOps<R>& r) noexcept {
        return spaceship(l, r) < 0;
    }
}
