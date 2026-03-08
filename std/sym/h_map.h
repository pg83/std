#pragma once

#include "h_table.h"

#include <std/mem/embed.h>
#include <std/typ/intrin.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace stl {
    template <typename T, typename K, typename H>
    class HashMap {
        struct Node: public HashTable::Node, public Embed<T> {
            using Embed<T>::Embed;
        };

        ObjList<Node> ol;
        HashTable st;

        T* insertNode(Node* value) {
            if (auto prev = (Node*)st.insert(value); prev) {
                ol.release(prev);
            }

            return &value->t;
        }

    public:
        ~HashMap() {
            if constexpr (!stdHasTrivialDestructor(Node)) {
                st.visit([](auto node) {
                    destruct((Node*)node);
                });
            }
        }

        T* find(K key) const {
            if (auto n = (Node*)st.find(H::hash(key)); n) {
                return &n->t;
            }

            return nullptr;
        }

        template <typename... A>
        T* insert(K key, A&&... a) {
            auto value = ol.make(forward<A>(a)...);

            return (value->key = H::hash(key), insertNode(value));
        }

        template <typename... A>
        T* insertKeyed(A&&... a) {
            auto value = ol.make(forward<A>(a)...);

            return (value->key = value->t.key(), insertNode(value));
        }

        T& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insert(key);
        }

        void erase(K key) {
            if (auto prev = (Node*)st.erase(H::hash(key)); prev) {
                ol.release(prev);
            }
        }

        template <typename F>
        void visit(F f) {
            st.visit([f](auto el) {
                f(((Node*)el)->t);
            });
        }
    };
}
