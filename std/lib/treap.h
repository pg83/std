#pragma once

namespace Std {
    struct Node {
        int priority;
        Node* left;
        Node* right;

        Node();

        virtual void* key() = 0;
    };

    class Treap {
        Node* root;

        void split(Node* t, Node* k, Node** l, Node** r) noexcept;
        Node* merge(Node* l, Node* r) noexcept;

    public:
        Treap()
            : root(nullptr)
        {
        }

        virtual bool cmp(void* a, void* b) = 0;

        Node* find(void* key) noexcept;
        void insert(Node* node) noexcept;
    };
}
