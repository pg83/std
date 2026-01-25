#pragma once

#include "stable.h"

#include <std/mem/pool.h>

namespace Std {
    template <typename T>
    class StringMap {
        Pool::Ref pool = Pool::fromMemory();
        StringTable st;

    public:
        inline T* find(const StringView& key) const noexcept {
            return (T*)st.find(key);
        }

        template <typename... A>
        inline T* insert(const StringView& key, A&&... a) {
            auto value = pool->make<T>(forward<A>(a)...);
            st.set(key, value);
            return value;
        }

        inline T& operator[](const StringView& key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insert(key);
        }
    };
}
