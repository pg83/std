#pragma once

namespace Std {
    struct TreapNode {
        int priority;
        TreapNode* left;
        TreapNode* right;

        TreapNode();

        virtual void* key() = 0;
    };

    class Treap {
        TreapNode* root;

        void split(TreapNode* t, TreapNode* k, TreapNode** l, TreapNode** r) noexcept;
        TreapNode* merge(TreapNode* l, TreapNode* r) noexcept;

    public:
        Treap()
            : root(nullptr)
        {
        }

        virtual bool cmp(void* a, void* b) = 0;

        TreapNode* find(void* key) noexcept;

        void erase(void* key) noexcept;
        void insert(TreapNode* node) noexcept;

        inline void erase(TreapNode* node) noexcept {
            erase(node->key());
        }
    };
}
