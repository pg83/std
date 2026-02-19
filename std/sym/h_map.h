#pragma once

#include <std/typ/support.h>
#include <std/mem/obj_pool.h>

namespace Std {
    template <typename T, typename K, typename S>
    class HashMap {
        ObjPool::Ref pool = ObjPool::fromMemory();
        S st;

    public:
        inline T* find(K key) const noexcept {
            return (T*)st.find(key);
        }

        template <typename... A>
        inline T* insert(K key, A&&... a) {
            auto value = pool->make<T>(forward<A>(a)...);
            st.set(key, value);
            return value;
        }

        inline T& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insert(key);
        }

        template <typename F>
        inline void visit(F f) {
            st.visit([f](void** el) {
                v(**(T**)el);
            });
        }

        inline void compactify() {
            auto np = ObjPool::fromMemory();

            st.compactify();
            st.visit([&np](void** el) {
                *el = np->make<T>(move(**(T**)el));
            });
            np.xchg(pool);
        }
    };
}
