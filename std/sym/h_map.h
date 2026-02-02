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

        inline void compactify() {
            struct Helper: public S::Iterator {
                ObjPool::Ref pool = ObjPool::fromMemory();

                void process(void** el) override {
                    *el = pool->make<T>(move(*(T*)*el));
                }
            };

            st.compactify();
            Helper it;
            st.forEach(it);
            pool.xchg(it.pool);
        }
    };
}
