#pragma once

#include <std/sys/types.h>

namespace Std {
    struct TreapVisitor;

    struct TreapNode {
        TreapNode* left = nullptr;
        TreapNode* right = nullptr;

        virtual void* key() const noexcept;

        void visit(TreapVisitor& vis);
        size_t height() const noexcept;
    };
}
