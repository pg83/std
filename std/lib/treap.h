#pragma once

#include "visitor.h"

#include <std/sys/types.h>

namespace Std {
    struct TreapNode;

    class Treap {
        TreapNode* root;

        void visitImpl(VisitorFace&& vis);
        void split(TreapNode* t, void* k, TreapNode** l, TreapNode** r) noexcept;

        virtual bool cmp(void* a, void* b) const noexcept = 0;

    public:
        inline Treap() noexcept
            : root(nullptr)
        {
        }

        template <typename V>
        inline void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((TreapNode*)ptr);
            }));
        }

        TreapNode* find(void* key) const noexcept;

        void erase(void* key) noexcept;
        void remove(TreapNode* node) noexcept;
        void insert(TreapNode* node) noexcept;
        void insertUnique(TreapNode* node) noexcept;

        size_t height() const noexcept;
        size_t length() const noexcept;
    };
}
