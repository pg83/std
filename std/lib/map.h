#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/typ/support.h>
#include <std/mem/obj_pool.h>
#include <std/rng/split_mix_64.h>

namespace Std {
    template <typename K, typename V>
    class Map: public Treap {
        struct Node: public TreapNode  {
            K k;
            V v;

            template <typename... A>
            inline Node(u64 prio, K key, A&&... a)
                : TreapNode(prio)
                , k(key)
                , v(forward<A>(a)...)
            {
            }

            void* key() const noexcept override {
                return (void*)&k;
            }
        };

        ObjPool::Ref pool = ObjPool::fromMemory();

        bool cmp(void* l, void* r) const noexcept override {
            return *(K*)l < *(K*)r;
        }

    public:
        inline V* find(K k) const noexcept {
            if (auto res = Treap::find((void*)&k); res) {
                return &(((Node*)res)->v);
            }

            return nullptr;
        }

        template <typename... A>
        inline V* insert(K key, A&&... a) {
            erase(key);
            auto node = pool->make<Node>(nextSplitMix64((size_t)pool->current()), key, forward<A>(a)...);
            Treap::insert(node);
            return &node->v;
        }

        inline V& operator[](K k) {
            if (auto res = find(k); res) {
                return *res;
            }

            return *insert(k);
        }

        inline void erase(K key) noexcept {
            Treap::erase((void*)&key);
        }
    };
}
