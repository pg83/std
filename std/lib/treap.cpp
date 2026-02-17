#include "treap.h"

#include <stdlib.h>

using namespace Std;

TreapNode::TreapNode()
    : priority(rand())
    , left(nullptr)
    , right(nullptr)
{
}

void Treap::split(TreapNode* t, TreapNode* k, TreapNode** l, TreapNode** r) noexcept {
    if (!t) {
        *l = nullptr;
        *r = nullptr;

        return;
    }

    if (cmp(t->key(), k->key())) {
        split(t->right, k, &t->right, r);
        *l = t;
    } else {
        split(t->left, k, l, &t->left);
        *r = t;
    }
}

TreapNode* Treap::merge(TreapNode* l, TreapNode* r) noexcept {
    if (!l) {
        return r;
    }

    if (!r) {
        return l;
    }

    if (l->priority > r->priority) {
        l->right = merge(l->right, r);

        return l;
    }

    r->left = merge(l, r->left);

    return r;
}

void Treap::insert(TreapNode* node) noexcept {
    TreapNode *l, *r;
    split(root, node, &l, &r);
    root = merge(merge(l, node), r);
}

TreapNode* Treap::find(void* key) noexcept {
    TreapNode* t = root;

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

void Treap::erase(void* key) noexcept {
    TreapNode** ptr = &root;

    while (*ptr) {
        TreapNode* current = *ptr;

        if (cmp(key, current->key())) {
            ptr = &current->left;
        } else if (cmp(current->key(), key)) {
            ptr = &current->right;
        } else {
            *ptr = merge(current->left, current->right);
            return;
        }
    }
}
