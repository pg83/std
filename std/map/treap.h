#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace Std {
    struct TreapNode;

    class Treap {
        TreapNode* root = nullptr;

        void visitImpl(VisitorFace&& vis);
        void split(TreapNode* t, void* k, TreapNode** l, TreapNode** r) noexcept;

        virtual bool cmp(void* a, void* b) const noexcept = 0;

    public:
        template <typename V>
        inline void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((TreapNode*)ptr);
            }));
        }

        TreapNode* find(void* key) const noexcept;
        TreapNode* erase(void* key) noexcept;
        TreapNode* remove(TreapNode* node) noexcept;

        void insert(TreapNode* node) noexcept;

        size_t height() const noexcept;
        size_t length() const noexcept;
    };
}
