#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/typ/support.h>
#include <std/mem/obj_pool.h>

namespace Std {
    template <typename K, typename V>
    class Map {
        struct Data: public Treap {
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
                return (void*)&k;
            }
        };

        ObjPool::Ref pool = ObjPool::fromMemory();
        Data map;

    public:
        inline V* find(K k) const noexcept {
            if (auto res = map.find((void*)&k); res) {
                return &(((Node*)res)->v);
            }

            return nullptr;
        }

        template <typename... A>
        inline V* insert(K key, A&&... a) {
            auto node = pool->make<Node>(key, forward<A>(a)...);
            map.insertUnique(node);
            return &node->v;
        }

        inline V& operator[](K k) {
            if (auto res = find(k); res) {
                return *res;
            }

            return *insert(k);
        }

        inline void erase(K key) noexcept {
            map.erase((void*)&key);
        }

        template <typename F>
        inline void visit(F v) {
            map.visit([&v](void* ptr) {
                v(((const Node*)ptr)->k, ((Node*)ptr)->v);
            });
        }
    };
}
