#include "treap.h"
#include "treap_node.h"

#include <std/lib/vector.h>
#include <std/rng/split_mix_64.h>

using namespace stl;

namespace {
    static u64 prio(const void* el) noexcept {
        return splitMix64((size_t)el);
    }

    static TreapNode* merge(TreapNode* l, TreapNode* r) noexcept {
        if (!l) {
            return r;
        } else if (!r) {
            return l;
        } else if (prio(l) > prio(r)) {
            return (l->right = merge(l->right, r), l);
        } else {
            return (r->left = merge(l, r->left), r);
        }
    }
}

void Treap::split(TreapNode* t, void* k, TreapNode** l, TreapNode** r) noexcept {
    if (!t) {
        *l = nullptr;
        *r = nullptr;
    } else if (cmp(t->key(), k)) {
        split(t->right, k, &t->right, r);
        *l = t;
    } else {
        split(t->left, k, l, &t->left);
        *r = t;
    }
}

void Treap::insert(TreapNode* node) noexcept {
    TreapNode* l;
    TreapNode* r;

    split(root, node->key(), &l, &r);
    root = merge(merge(l, node), r);
}

TreapNode* Treap::find(void* key) const noexcept {
    auto t = root;

    while (t) {
        if (auto tkey = t->key(); cmp(key, tkey)) {
            t = t->left;
        } else if (cmp(tkey, key)) {
            t = t->right;
        } else {
            return t;
        }
    }

    return nullptr;
}

TreapNode* Treap::erase(void* key) noexcept {
    auto ptr = &root;

    while (auto current = *ptr) {
        if (auto ckey = current->key(); cmp(key, ckey)) {
            ptr = &current->left;
        } else if (cmp(ckey, key)) {
            ptr = &current->right;
        } else {
            return (*ptr = merge(current->left, current->right), current);
        }
    }

    return nullptr;
}

TreapNode* Treap::remove(TreapNode* node) noexcept {
    return erase(node->key());
}

void Treap::visitImpl(VisitorFace&& vis) {
    TreapNode* buf[64];

    visitImpl(vis, buf, 64);
}

void Treap::visitImpl(VisitorFace& vis, TreapNode** buf, size_t count) {
    if (!root) {
        return;
    }

    size_t n = 0;

    auto collect = makeVisitor([&](void* ptr) {
        if (n < count) {
            buf[n++] = (TreapNode*)ptr;
        } else {
            throw count;
        }
    });

    try {
        root->visit(collect);
    } catch (size_t c) {
        Vector<TreapNode*> newBuf(c * 2);

        return visitImpl(vis, newBuf.mutData(), count * 2);
    }

    for (size_t i = 0; i < n; i++) {
        vis.visit(buf[i]);
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
