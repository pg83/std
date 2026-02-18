#pragma once

namespace Std {
    struct TreapVisitor;

    struct TreapNode {
        TreapNode* left = nullptr;
        TreapNode* right = nullptr;

        virtual void* key() const noexcept;

        void visit(TreapVisitor& vis);
        unsigned height() const noexcept;
    };
}
