#pragma once

namespace Std {
    struct TreapVisitor;

    struct TreapNode {
        int priority;
        TreapNode* left;
        TreapNode* right;

        TreapNode() noexcept;

        virtual void* key() const noexcept;

        void visit(TreapVisitor& vis);
    };
}
