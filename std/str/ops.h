#pragma once

#include <std/sys/crt.h>
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

    // compare ops
    template <typename L, typename R>
    inline int spaceship(const StringOps<L>& l, const StringOps<R>& r) noexcept {
        const auto ll = l.length();
        const auto rl = r.length();
        const auto rr = memCmp(l.data(), r.data(), ll < rl ? ll : rl);

        return rr ? rr : (ll < rl ? -1 : (ll == rl ? 0 : 1));
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
