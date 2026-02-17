#pragma once

namespace Std {
    struct TreapNode;

    struct TreapVisitor {
        virtual void visit(void*) = 0;
    };

    class Treap {
        TreapNode* root;

        void visitImpl(TreapVisitor&& vis);
        void split(TreapNode* t, TreapNode* k, TreapNode** l, TreapNode** r) noexcept;

    public:
        inline Treap() noexcept
            : root(nullptr)
        {
        }

        virtual bool cmp(void* a, void* b) const noexcept = 0;

        template <typename V>
        inline void visit(V v) {
            struct H: public TreapVisitor {
                V v;

                inline H(V vv)
                    : v(vv)
                {
                }

                virtual void visit(void* el) {
                    v(el);
                }
            };

            visitImpl(H(v));
        }

        TreapNode* find(void* key) const noexcept;

        void erase(void* key) noexcept;
        void remove(TreapNode* node) noexcept;
        void insert(TreapNode* node) noexcept;
    };
}
