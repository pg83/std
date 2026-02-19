#pragma once

namespace Std {
    struct VisitorFace;

    struct TreapNode {
        TreapNode* left = nullptr;
        TreapNode* right = nullptr;

        virtual void* key() const noexcept;

        void visit(VisitorFace& vis);
        unsigned height() const noexcept;
    };
}
