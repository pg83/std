#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/mem/new.h>
#include <std/typ/support.h>
#include <std/mem/free_list.h>

namespace Std {
    template <typename K, typename V>
    class Map {
        static inline void* tov(const K& k) noexcept {
            return (void*)&k;
        }

        struct Data: public Treap {
            bool cmp(void* l, void* r) const noexcept override {
                return *(K*)l < *(K*)r;
            }
        };

        struct Node: public TreapNode, public Newable {
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

        FreeList::Ref fl = FreeList::fromMemory(sizeof(Node));
        Data map;

        template <typename... A>
        inline V* insertNew(K key, A&&... a) {
            auto node = new (fl->allocate()) Node(key, forward<A>(a)...);
            map.insert(node);
            return &node->v;
        }

    public:
        inline ~Map() noexcept {
            map.visit([](TreapNode* ptr) {
                ((Node*)ptr)->~Node();
            });
        }

        inline V* find(K k) const noexcept {
            if (auto res = map.find(tov(k)); res) {
                return &(((Node*)res)->v);
            }

            return nullptr;
        }

        template <typename... A>
        inline V* insert(K key, A&&... a) {
            if (auto prev = find(key); prev) {
                return (*prev = V(forward<A>(a)...), prev);
            }

            return insertNew(key, forward<A>(a)...);
        }

        inline V& operator[](K k) {
            if (auto res = find(k); res) {
                return *res;
            }

            return *insertNew(k);
        }

        inline void erase(K key) noexcept {
            if (auto res = map.erase(tov(key)); res) {
                fl->release((Node*)res);
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
