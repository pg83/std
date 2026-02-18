#include "treap.h"
#include "treap_node.h"

using namespace Std;

namespace {
    static inline TreapNode* merge(TreapNode* l, TreapNode* r) noexcept {
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

void Treap::insert(TreapNode* node) noexcept {
    TreapNode *l, *r;
    split(root, node, &l, &r);
    root = merge(merge(l, node), r);
}

TreapNode* Treap::find(void* key) const noexcept {
    auto t = root;

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
    auto ptr = &root;

    while (auto current = *ptr) {
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

void Treap::remove(TreapNode* node) noexcept {
    erase(node->key());
}

void Treap::visitImpl(TreapVisitor&& vis) {
    if (root) {
        root->visit(vis);
    }
}

size_t Treap::length() const noexcept {
    size_t res = 0;

    ((Treap*)this)->visit([&res](void*) {
        res += 1;
    });

    return res;
}

size_t Treap::height() const noexcept {
    if (root) {
        return root->height();
    }

    return 0;
}
