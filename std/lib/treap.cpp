#include "treap.h"

#include <stdlib.h>

using namespace Std;

Node::Node()
    : priority(rand())
    , left(nullptr)
    , right(nullptr)
{
}

void Treap::split(Node* t, Node* k, Node** l, Node** r) noexcept {
    if (!t) {
        *l = nullptr;
        *r = nullptr;

        return;
    }

    if (cmp(t->key(), k)) {
        split(t->right, k, &t->right, r);
        *l = t;
    } else {
        split(t->left, k, l, &t->left);
        *r = t;
    }
}

Node* Treap::merge(Node* l, Node* r) noexcept {
    if (!l || !r) {
        return l ? l : r;
    }

    if (l->priority > r->priority) {
        l->right = merge(l->right, r);

        return l;
    }

    r->left = merge(l, r->left);

    return r;
}

void Treap::insert(Node* node) noexcept {
    Node *l, *r;
    split(root, node, &l, &r);
    root = merge(merge(l, node), r);
}

void Treap::erase(Node* node) noexcept {
    Node *l, *m, *r;

    split(root, node, &l, &r);

    if (r) {
        split(r, node, &m, &r);
    }

    root = merge(l, r);
}

Node* Treap::find(void* key) noexcept {
    Node* t = root;

    while (t) {
        if (cmp(key, t->key())) {
            t = t->left;
        } else if (cmp(t->key(), key)) {
            t = t->right;
        } else {
            return t;
        }
    }

    return nullptr;
}
