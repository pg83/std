#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/typ/support.h>
#include <std/mem/obj_pool.h>

namespace Std {
    template <typename K, typename V>
    class Map: public Treap {
        struct Node: public TreapNode, public Embed<V> {
            using Embed<V>::Embed;

            K k;

            void* key() const noexcept override {
                return &k;
            }
        };

        ObjPool::Ref pool = ObjPool::fromMemory();

        bool cmp(void* l, void* r) const noexcept override {
            return *(K*)l == *(K*)r;
        }

    public:
        inline V* find(const K& k) const noexcept {
            if (auto res = find(&k); res) {
                return *((Node*)res)->t;
            }

            return nullptr;
        }

        template <typename... A>
        inline V* insert(K key, A&&... a) {
            auto node = pool->make<Node>(forward<A>(a)...);
            insert(node);
            return &node->t;
        }

        inline V& operator[](const K& k) {
            if (auto res = find(k); res) {
                return *res;
            }

            return insert(k);
        }
    };
}
