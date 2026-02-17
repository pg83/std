#pragma once

namespace Std {
    struct TreapVisitor;

    struct TreapNode {
        int priority;
        TreapNode* left;
        TreapNode* right;

        TreapNode() noexcept;

        virtual void* key() noexcept;

        void visit(TreapVisitor& vis);
    };
}
