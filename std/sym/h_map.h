#pragma once

#include "h_table.h"

#include <std/typ/intrin.h>
#include <std/typ/support.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace Std {
    template <typename T, typename K, typename H>
    class HashMap {
        struct Node: public HashTable::Node {
            T v;

            template <typename... A>
            inline Node(K k, A&&... a)
                : v(forward<A>(a)...)
            {
                this->key = H::hash(k);
            }
        };

        ObjList<Node> ol;
        HashTable st;

    public:
        inline ~HashMap() {
            if constexpr (!stdHasTrivialDestructor(Node)) {
                st.visit([](auto node) {
                    destruct((Node*)node);
                });
            }
        }

        inline T* find(K key) const noexcept {
            if (auto n = (Node*)st.find(H::hash(key)); n) {
                return &n->v;
            }

            return nullptr;
        }

        template <typename... A>
        inline T* insert(K key, A&&... a) {
            auto value = ol.make(key, forward<A>(a)...);

            if (auto prev = (Node*)st.insert(value); prev) {
                ol.release(prev);
            }

            return &value->v;
        }

        inline T& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insert(key);
        }

        inline void erase(K key) noexcept {
            if (auto prev = (Node*)st.erase(H::hash(key)); prev) {
                ol.release(prev);
            }
        }

        template <typename F>
        inline void visit(F f) {
            st.visit([f](auto el) {
                f(((Node*)el)->v);
            });
        }
    };
}
