#pragma once

#include <std/typ/support.h>
#include <std/mem/obj_list.h>

namespace Std {
    template <typename T, typename K, typename S>
    class HashMap {
        ObjList<T> ol;
        S st;

    public:
        inline ~HashMap() {
            visit([](auto& node) {
                node.~T();
            });
        }

        inline T* find(K key) const noexcept {
            return (T*)st.find(key);
        }

        template <typename... A>
        inline T* insert(K key, A&&... a) {
            auto value = ol.make(forward<A>(a)...);

            if (auto prev = (T*)st.set(key, value); prev) {
                ol.release(prev);
            }

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
                f(**(T**)el);
            });
        }

        inline void compactify() {
            st.compactify();
        }
    };
}
