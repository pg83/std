#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace stl {
    struct TreapNode;

    class Treap {
        TreapNode* root = nullptr;

        void visitImpl(VisitorFace&& vis);
        void split(TreapNode* t, void* k, TreapNode** l, TreapNode** r);

        virtual bool cmp(void* a, void* b) const = 0;

    public:
        template <typename V>
        void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((TreapNode*)ptr);
            }));
        }

        TreapNode* find(void* key) const;
        TreapNode* erase(void* key);
        TreapNode* remove(TreapNode* node);

        void insert(TreapNode* node);

        size_t height() const;
        size_t length() const;
    };
}
