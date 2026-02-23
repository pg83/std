#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/typ/intrin.h>
#include <std/typ/support.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace Std {
    template <typename K, typename V>
    class Map {
        static inline void* tov(const K& k) noexcept {
            return (void*)&k;
        }

        struct Impl: public Treap {
            bool cmp(void* l, void* r) const noexcept override {
                return *(K*)l < *(K*)r;
            }
        };

        struct Node: public TreapNode {
            K k;
            V v;

            template <typename... A>
            inline Node(K key, A&&... a)
                : k(key)
                , v(forward<A>(a)...)
            {
            }

            void* key() const noexcept override {
                return tov(k);
            }
        };

        ObjList<Node> ol;
        Impl map;

        template <typename... A>
        inline V* insertNew(K key, A&&... a) {
            auto node = ol.make(key, forward<A>(a)...);
            map.insert(node);
            return &node->v;
        }

    public:
        inline ~Map() noexcept {
            if constexpr (!stdHasTrivialDestructor(Node)) {
                map.visit([](auto ptr) {
                    destruct((Node*)ptr);
                });
            }
        }

        inline V* find(K k) const noexcept {
            if (auto res = (Node*)map.find(tov(k)); res) {
                return &res->v;
            }

            return nullptr;
        }

        template <typename... A>
        inline V* insert(K key, A&&... a) {
            return (erase(key), insertNew(key, forward<A>(a)...));
        }

        inline V& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insertNew(key);
        }

        inline void erase(K key) noexcept {
            if (auto res = (Node*)map.erase(tov(key)); res) {
                ol.release(res);
            }
        }

        template <typename F>
        inline void visit(F v) {
            map.visit([v](TreapNode* ptr) {
                v(((const Node*)ptr)->k, ((Node*)ptr)->v);
            });
        }
    };
}
