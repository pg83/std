#pragma once

namespace Std {
    struct TreapNode {
        int priority;
        TreapNode* left;
        TreapNode* right;

        TreapNode() noexcept;

        virtual void* key() noexcept;
    };

    class Treap {
        TreapNode* root;

        void split(TreapNode* t, TreapNode* k, TreapNode** l, TreapNode** r) noexcept;
        TreapNode* merge(TreapNode* l, TreapNode* r) noexcept;

    public:
        Treap() noexcept
            : root(nullptr)
        {
        }

        virtual bool cmp(void* a, void* b) noexcept = 0;

        TreapNode* find(void* key) noexcept;

        void erase(void* key) noexcept;
        void insert(TreapNode* node) noexcept;

        inline void erase(TreapNode* node) noexcept {
            erase(node->key());
        }
    };
}
