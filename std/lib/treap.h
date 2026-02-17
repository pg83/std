#pragma once

namespace Std {
    struct TreapNode;

    class Treap {
        TreapNode* root;

        void split(TreapNode* t, TreapNode* k, TreapNode** l, TreapNode** r) noexcept;
        TreapNode* merge(TreapNode* l, TreapNode* r) noexcept;

    public:
        Treap() noexcept
            : root(nullptr)
        {
        }

        virtual bool cmp(void* a, void* b) const noexcept = 0;

        TreapNode* find(void* key) const noexcept;

        void erase(void* key) noexcept;
        void erase(TreapNode* node) noexcept;
        void insert(TreapNode* node) noexcept;
    };
}
