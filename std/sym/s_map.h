#pragma once

#include "s_table.h"

#include <std/mem/obj_pool.h>

namespace Std {
    template <typename T>
    class SymbolMap {
        ObjPool::Ref pool = ObjPool::fromMemory();
        SymbolTable st;

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
