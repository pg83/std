#pragma once

namespace stl {
    struct VisitorFace;

    struct TreapNode {
        TreapNode* left = nullptr;
        TreapNode* right = nullptr;

        virtual void* key() const;

        void visit(VisitorFace& vis);
        unsigned height() const;
    };
}
